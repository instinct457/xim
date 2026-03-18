/* xim.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>
#include <dbus/dbus.h>

#include "config.h"

Display *dpy;
int active_inhibitors = 0;
int cookie_counter = 0;

void
spawn(const char *cmd) 
{
    if (fork() == 0) {
        if (dpy) close(ConnectionNumber(dpy));
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        fprintf(stderr, "[ERR]: could not run '%s'\n", cmd);
        exit(1);
    }
}

void 
handle_dbus_messages(DBusConnection *conn) 
{
    dbus_connection_read_write(conn, 0);
    DBusMessage *msg;
    
    while ((msg = dbus_connection_pop_message(conn)) != NULL) {
        if (dbus_message_is_method_call(msg, "org.freedesktop.ScreenSaver", "Inhibit")) {
            active_inhibitors++;
            cookie_counter++;
            
            /* we have to reply with a cookie */
            DBusMessage *reply = dbus_message_new_method_return(msg);
            if (reply) {
                dbus_message_append_args(reply, DBUS_TYPE_UINT32, &cookie_counter, DBUS_TYPE_INVALID);
                dbus_connection_send(conn, reply, NULL);
                dbus_message_unref(reply);
            }
        } 
        else if (dbus_message_is_method_call(msg, "org.freedesktop.ScreenSaver", "UnInhibit")) {
            if (active_inhibitors > 0) active_inhibitors--;
            
            DBusMessage *reply = dbus_message_new_method_return(msg);
            if (reply) {
                dbus_connection_send(conn, reply, NULL);
                dbus_message_unref(reply);
            }
        }
        dbus_message_unref(msg);
    }
}

int 
main() 
{
    int sync_event_base, sync_error_base, ncounters;
    XSyncSystemCounter *counters;
    XSyncCounter idle_counter = None;
    
    int num_rules = sizeof(rules) / sizeof(rules[0]);
    XSyncAlarm rule_alarms[num_rules];
    XSyncAlarm reset_alarm;

    signal(SIGCHLD, SIG_IGN);

    DBusError dbus_err;
    dbus_error_init(&dbus_err);
    DBusConnection *dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_err);
    if (!dbus_conn) {
        fprintf(stderr, "[ERR]: Could not connect to DBus.\n");
        return 1;
    }

    /* claim screensaver name */
    dbus_bus_request_name(dbus_conn, "org.freedesktop.ScreenSaver", DBUS_NAME_FLAG_REPLACE_EXISTING, &dbus_err);

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
    	fprintf(stderr, "[ERR]: Could not connect to X11.\n");
    	return 1;
    }
    
    if (!XSyncQueryExtension(dpy, &sync_event_base, &sync_error_base)) {
    	fprintf(stderr, "[ERR]: XSync extension is not supported.\n");
    	return 1;
    }

    counters = XSyncListSystemCounters(dpy, &ncounters);
    for (int i = 0; i < ncounters; i++) {
        if (strcmp(counters[i].name, "IDLETIME") == 0) {
            idle_counter = counters[i].counter; break;
        }
    }
    
    XSyncFreeSystemCounterList(counters);
    if (idle_counter == None) {
    	fprintf(stderr, "[ERR]: Could not find IDLETIME counter.\n");
    	return 1;
    }

    XSyncAlarmAttributes attr;
    attr.trigger.counter = idle_counter;
    attr.trigger.value_type = XSyncAbsolute;
    attr.events = True;
    XSyncIntToValue(&attr.delta, 0);

    unsigned long alarm_mask = XSyncCACounter | XSyncCAValueType | 
                               XSyncCAValue | XSyncCATestType | 
                               XSyncCADelta | XSyncCAEvents;

    XSyncIntToValue(&attr.trigger.wait_value, 1);
    attr.trigger.test_type = XSyncNegativeTransition;
    reset_alarm = XSyncCreateAlarm(dpy, alarm_mask, &attr);

    attr.trigger.test_type = XSyncPositiveTransition;
    for (int i = 0; i < num_rules; i++) {
        XSyncIntsToValue(&attr.trigger.wait_value, rules[i].timeout * 1000, 0);
        rule_alarms[i] = XSyncCreateAlarm(dpy, alarm_mask, &attr);
    }

    /* poll main loop */
    struct pollfd fds[2];
    fds[0].fd = ConnectionNumber(dpy); 					/* x11 file descriptor */
    fds[0].events = POLLIN;
    dbus_connection_get_unix_fd(dbus_conn, &fds[1].fd); /* dbus file descriptor */
    fds[1].events = POLLIN;

    while (1) {
    	XFlush(dpy);
        /* program hangs until x11 or dbus send something */
        poll(fds, 2, -1);

        if (fds[1].revents & POLLIN) {
            handle_dbus_messages(dbus_conn);
        }

        while (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            
            if (ev.type == sync_event_base + XSyncAlarmNotify) {
                XSyncAlarmNotifyEvent *alarm_ev = (XSyncAlarmNotifyEvent *)&ev;

                if (alarm_ev->alarm == reset_alarm) {
                    for (int i = 0; i < num_rules; i++) {
                        if (rules[i].active) {
                            if (rules[i].on_resume) spawn(rules[i].on_resume);
                            rules[i].active = 0;
                        }
                        /* resetting timers */
                        XSyncIntsToValue(&attr.trigger.wait_value, rules[i].timeout * 1000, 0);
                        XSyncChangeAlarm(dpy, rule_alarms[i], XSyncCAValue, &attr);
                    }
                    XSyncIntToValue(&attr.trigger.wait_value, 1);
                    XSyncChangeAlarm(dpy, reset_alarm, XSyncCAValue, &attr);
                } 
                else {
                    for (int i = 0; i < num_rules; i++) {
                        if (alarm_ev->alarm == rule_alarms[i] && !rules[i].active) {
                            if (active_inhibitors > 0) {
                                /* sleep is inhibited. we move alarms 20 secs away */
                                XSyncValue counter_val;
                                XSyncQueryCounter(dpy, idle_counter, &counter_val);
                                int current_idle = XSyncValueLow32(counter_val) / 1000;
                                
                                XSyncIntsToValue(&attr.trigger.wait_value, (current_idle + 20) * 1000, 0);
                                XSyncChangeAlarm(dpy, rule_alarms[i], XSyncCAValue, &attr);
                                continue;
                            }
                            if (rules[i].on_timeout) spawn(rules[i].on_timeout);
                            rules[i].active = 1;
                        }
                    }
                }
            }
        }
    }
    XCloseDisplay(dpy);
    return 0;

}
