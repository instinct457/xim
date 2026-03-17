#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>

#include "config.h"

Display *dpy;

void 
spawn(const char *cmd) 
{
    if (fork() == 0) {
        if (dpy) close(ConnectionNumber(dpy));
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        fprintf(stderr, "[ERR]: could not run: '%s'\n", cmd);
        exit(1);
    }
}

int 
main() 
{
    int sync_event_base, sync_error_base;
    int ncounters;
    XSyncSystemCounter *counters;
    XSyncCounter idle_counter = None;
    
    int num_rules = sizeof(rules) / sizeof(rules[0]);
    XSyncAlarm rule_alarms[num_rules];
    XSyncAlarm reset_alarm;

    signal(SIGCHLD, SIG_IGN);

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "[ERR]: could not connect to display.\n");
        return 1;
    }

    if (!XSyncQueryExtension(dpy, &sync_event_base, &sync_error_base)) {
        fprintf(stderr, "[ERR]: XSync extension unsupported.\n");
        return 1;
    }

    counters = XSyncListSystemCounters(dpy, &ncounters);
    for (int i = 0; i < ncounters; i++) {
        if (strcmp(counters[i].name, "IDLETIME") == 0) {
            idle_counter = counters[i].counter;
            break;
        }
    }
    XSyncFreeSystemCounterList(counters);

    if (idle_counter == None) {
        fprintf(stderr, "[ERR]: IDLETIME counter not found.\n");
        return 1;
    }

    XSyncAlarmAttributes attr;
    attr.trigger.counter = idle_counter;
    attr.trigger.value_type = XSyncAbsolute;
    attr.events = True;

    /* set delta to 0 to disable alarm auto-repetition */
    XSyncIntToValue(&attr.delta, 0);

    /* adding XSyncCADelta to mask */
    unsigned long alarm_mask = XSyncCACounter | XSyncCAValueType | 
                               XSyncCAValue | XSyncCATestType | 
                               XSyncCADelta | XSyncCAEvents;

    /* reset alarm creation */
    XSyncIntToValue(&attr.trigger.wait_value, 1);
    attr.trigger.test_type = XSyncNegativeTransition;
    reset_alarm = XSyncCreateAlarm(dpy, alarm_mask, &attr);

    /* timeout alarm creation */
    attr.trigger.test_type = XSyncPositiveTransition;
    for (int i = 0; i < num_rules; i++) {
        XSyncIntsToValue(&attr.trigger.wait_value, rules[i].timeout * 1000, 0);
        rule_alarms[i] = XSyncCreateAlarm(dpy, alarm_mask, &attr);
    }

    /* event-driven loop */
    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);

        if (ev.type == sync_event_base + XSyncAlarmNotify) {
            XSyncAlarmNotifyEvent *alarm_ev = (XSyncAlarmNotifyEvent *)&ev;

            /* user activity */
            if (alarm_ev->alarm == reset_alarm) {
                for (int i = 0; i < num_rules; i++) {
                    if (rules[i].active) {
                        if (rules[i].on_resume) spawn(rules[i].on_resume);
                        rules[i].active = 0;
                    }
                    /* timed-out alarms are inactive.
                    let's rearm them */
                    XSyncIntsToValue(&attr.trigger.wait_value, rules[i].timeout * 1000, 0);
                    XSyncChangeAlarm(dpy, rule_alarms[i], XSyncCAValue, &attr);
                }
                /* riarming reset alarm too */
                XSyncIntToValue(&attr.trigger.wait_value, 1);
                XSyncChangeAlarm(dpy, reset_alarm, XSyncCAValue, &attr);
            } 
            /* alarm timed-out */
            else {
                for (int i = 0; i < num_rules; i++) {
                    if (alarm_ev->alarm == rule_alarms[i] && !rules[i].active) {
                        if (rules[i].on_timeout) spawn(rules[i].on_timeout);
                        rules[i].active = 1;
                        /* alarm is not active. reset_alarm will rearm it */
                    }
                }
            }
        }
    }

    XCloseDisplay(dpy);
    return 0;
}
