xim - X Idle Manager is a suckless-inspired idle manager for X.

### Why

Having a laptop with an OLED display I have slightly different needs than the average user with a window manager, let alone a Suckless setup. Having used Wayland (Sway) for some time I really liked the swayidle approach to idle management. The ability to specify a command to execute on idle opens you to all sorts of possibilities, even opening a video on your browser.  

Unfortunately I couldn't replicate a similar setup on X11 (let alone finding a similar program), so I decided to just make my own version of swayidle, the Suckless way.

### How

xim is very simple. It uses the `Sync` extension to create various alarms triggered by the `IDLECOUNTER` counter.

When a rule is triggered, the string present on the `ON_TIMEOUT` rule is triggered, and when the `IDLECOUNTER` reports new user activity, the `ON_RESUME` rule is triggered. Xim also supports the DBus Inhibit message and disables all DPMS timers on startup.

### How to

Being a suckless-inspired program, configuration is made by editing the `config.h` file, which will be generated the first time you run `make`, or by manually running `make config.h`. The file is hopefully self-explicable and easy to configure.

### Screen locking on suspend

Since xim doesn't manage screen locking by itself, the system won't lock on suspend. You can implement this feature by creating a system service. Here's a systemd unit taken from the [slock Arch Wiki page](https://wiki.archlinux.org/title/Slock#Lock_on_suspend):

```
[Unit]
Description=Lock X session using slock for user %i
Before=sleep.target

[Service]
User=%i
Environment=DISPLAY=:0
ExecStart=/usr/local/bin/slock

[Install]
WantedBy=sleep.target
```