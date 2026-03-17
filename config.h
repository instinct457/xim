/* * Rule structure:
 * 1. timeout		Inactivity time in seconds before triggering the command.
 * 2. on_timeout	Command to execute when the timeout is reached.
 * 3. on_resume		Command to execute when user activity resumes (can be NULL).
 * 4. active		Internal state flag. ALWAYS leave this as 0.
 */
typedef struct {
    int timeout;
    const char *on_timeout;
    const char *on_resume;
    int active;
} Rule;

/* Define your idle rules here. The rules will be evaluated concurrently by XSync alarms. */
static Rule rules[] = {
    /* Timeout,	Command on timeout,		Command on resume,		State */

	/* Dims the display to 5% brightness and powers off keyboard backlight after 30 seconds of inactivity */
	{ 30,  		"brightnessctl -s set 5%; brightnessctl -sd platform::kbd_backlight set 0",	"brightnessctl -r; brightnessctl -rd platform::kbd_backlight",	0 },
    
    /* Turn off the monitor after 1 minute of inactivity */
    { 60,  		"xset dpms force off",	"xset dpms force on",	0 },
    
    /* Lock the screen with 'slock' after 1.5 minutes (90 seconds) of */
    { 90,  		"slock",				NULL,					0 },
    
    /* Suspend the system after 2 minutes */
    { 120, 		"systemctl suspend",	NULL,					0 },
};
