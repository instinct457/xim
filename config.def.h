/* config.def.h - Default configuration for xim */

/* * Rule structure:
 * 1. timeout:   Inactivity time in seconds before triggering the command.
 * 2. on_timeout: Command to execute when the timeout is reached.
 * 3. on_resume:  Command to execute when user activity resumes (can be NULL).
 * 4. active:     Internal state flag. ALWAYS leave this as 0.
 */
typedef struct {
    int timeout;
    const char *on_timeout;
    const char *on_resume;
    int active;
} Rule;

/* * Define your idle rules here. 
 * The rules will be evaluated concurrently by XSync alarms.
 */
static Rule rules[] = {
    /* Timeout,   Command on timeout,         Command on resume,        State */
    
    /* Turn off the monitor after 3 minutes (180 seconds) of inactivity */
    { 180,        "xset dpms force off",      "xset dpms force on",     0 },
    
    /* Lock the screen with 'slock' after 5 minutes (300 seconds) */
    { 300,        "slock",                    NULL,                     0 },
    
    /* Suspend the system after 10 minutes (600 seconds) */
    { 600,        "systemctl suspend",        NULL,                     0 },
};
