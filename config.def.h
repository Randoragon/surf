/* modifier 0 means no modifier */
static int surfuseragent    = 1;  /* Append Surf version to default WebKit user agent */
static char *fulluseragent  = ""; /* Or override the whole user agent string */
static char *scriptfile     = "~/.local/share/surf/script.js";
static char *styledir       = "~/.local/share/surf/styles/";
static char *certdir        = "~/.local/share/surf/certificates/";
static char *cachedir       = "~/.local/share/surf/cache/";
static char *cookiefile     = "~/.local/share/surf/cookies.txt";
static char *dldir          = "~/Downloads/";
static char *dlstatus       = "~/.local/share/surf/dlstatus/";
static char *searchengine   = "https://duckduckgo.com/?q=";

/* Webkit default features */
/* Highest priority value will be used.
 * Default parameters are priority 0
 * Per-uri parameters are priority 1
 * Command parameters are priority 2
 */
static Parameter defconfig[ParameterLast] = {
    /* parameter                    Arg value       priority */
    [AcceleratedCanvas]   =       { { .i = 1 },     },
    [AccessMicrophone]    =       { { .i = 0 },     },
    [AccessWebcam]        =       { { .i = 0 },     },
    [Certificate]         =       { { .i = 0 },     },
    [CaretBrowsing]       =       { { .i = 0 },     },
    [CookiePolicies]      =       { { .v = "@Aa" }, },
    [DefaultCharset]      =       { { .v = "UTF-8" }, },
    [DiskCache]           =       { { .i = 1 },     },
    [DNSPrefetch]         =       { { .i = 0 },     },
    [FileURLsCrossAccess] =       { { .i = 0 },     },
    [FontSize]            =       { { .i = 16 },    },
    [FrameFlattening]     =       { { .i = 0 },     },
    [Geolocation]         =       { { .i = 0 },     },
    [HideBackground]      =       { { .i = 0 },     },
    [Inspector]           =       { { .i = 1 },     },
    [Java]                =       { { .i = 1 },     },
    [JavaScript]          =       { { .i = 0 },     },
    [KioskMode]           =       { { .i = 0 },     },
    [LoadImages]          =       { { .i = 1 },     },
    [MediaManualPlay]     =       { { .i = 1 },     },
    [Plugins]             =       { { .i = 1 },     },
    [PreferredLanguages]  =       { { .v = (char *[]){ NULL } }, },
    [RunInFullscreen]     =       { { .i = 0 },     },
    [ScrollBars]          =       { { .i = 1 },     },
    [ShowIndicators]      =       { { .i = 1 },     },
    [SiteQuirks]          =       { { .i = 1 },     },
    [SmoothScrolling]     =       { { .i = 1 },     },
    [SpellChecking]       =       { { .i = 0 },     },
    [SpellLanguages]      =       { { .v = ((char *[]){ "en_US", NULL }) }, },
    [StrictTLS]           =       { { .i = 1 },     },
    [Style]               =       { { .i = 1 },     },
    [WebGL]               =       { { .i = 0 },     },
    [ZoomLevel]           =       { { .f = 1.0 },   },
};

static UriParameters uriparams[] = {
    { "(://|\\.)suckless\\.org(/|$)", {
      [JavaScript] = { { .i = 0 }, 1 },
      [Plugins]    = { { .i = 0 }, 1 },
    }, },
};

static SearchEngine searchengines[] = {
    { "g",   "http://www.google.com/search?q=%s"    },
    { "aw",  "http://wiki.archlinux.org/?search=%s" },
    { "i",   "http://duckduckgo.com/?q=%s&t=hk&iax=images&ia=images" },
    { "yt",   "http://youtube.com/results?search_query=%s" },
};

/* default window size: width, height */
static int winsize[] = { 800, 600 };

static WebKitFindOptions findopts = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                                    WEBKIT_FIND_OPTIONS_WRAP_AROUND;

#define PROMPT_GO   "Go:"
#define PROMPT_FIND "Find:"

/* SETPROP(readprop, setprop, prompt)*/
#define SETPROP(r, s, p) { \
        .v = (const char *[]){ "/bin/sh", "-c", \
             "prop=\"$(printf '%b' \"$(xprop -id $1 $2 " \
             "| sed \"s/^$2(STRING) = //;s/^\\\"\\(.*\\)\\\"$/\\1/\" && cat ~/.config/surf/bookmarks)\" " \
             "| dmenu -l 10 -p \"$4\" -w $1)\" && " \
             "xprop -id $1 -f $3 8s -set $3 \"$prop\"", \
             "surf-setprop", winid, r, s, p, NULL \
        } \
}

#define DLSTATUS { \
        .v = (const char *[]){ "st", "-n", "surf-dl", "-e", "/bin/sh", "-c",\
            "while true; do cat $1/* 2>/dev/null || echo \"currently no downloads\";"\
            "A=; read A; "\
            "if [ $A = \"clean\" ]; then rm $1/*; fi; clear; done",\
            "surf-dlstatus", dlstatus, NULL } \
}

/* DOWNLOAD(URI, referer) */
#define DOWNLOAD(u, r) { \
        .v = (const char *[]){ "st", "-e", "/bin/sh", "-c",\
             "curl -g -L -J -O -A \"$1\" -b \"$2\" -c \"$2\"" \
             " -e \"$3\" \"$4\"; read", \
             "surf-download", useragent, cookiefile, r, u, NULL \
        } \
}

/* PLUMB(URI) */
/* This called when some URI which does not begin with "about:",
 * "http://" or "https://" should be opened.
 */
#define PLUMB(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "xdg-open \"$0\"", u, NULL \
        } \
}

/* VIDEOPLAY(URI) */
#define VIDEOPLAY(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "mpv --really-quiet \"$0\"", u, NULL \
        } \
}

/* BM_ADD(readprop) */
#define BM_ADD(r) {\
        .v = (const char *[]){ "/bin/sh", "-c", "surf-script-addbm $0 $1", winid, r, NULL } \
}
/* BM_RM(readprop) */
#define BM_RM(r) {\
        .v = (const char *[]){ "/bin/sh", "-c", "surf-script-rmbm $0 $1", winid, r, NULL } \
}


/* styles */
/*
 * The iteration will stop at the first match, beginning at the beginning of
 * the list.
 */
static SiteSpecific styles[] = {
    /* regexp               file in $styledir */
    { ".*",                 "default.css" },
};

/* certificates */
/*
 * Provide custom certificate for urls
 */
static SiteSpecific certs[] = {
    /* regexp               file in $certdir */
    { "://suckless\\.org/", "suckless.org.crt" },
};

#define MODKEY GDK_CONTROL_MASK

static char *linkselect_curwin [] = { "/bin/sh", "-c",
    "surf-script-linkselect $0 'Follow' | xargs -r xprop -id $0 -f _SURF_GO 8s -set _SURF_GO", winid, NULL
};
static char *linkselect_newwin [] = { "/bin/sh", "-c",
    "surf-script-linkselect $0 'Follow (new window)' | xargs -r surf", winid, NULL
};
static char *editsource[] = { "/bin/sh", "-c", "surf-script-editsource", NULL };

/* hotkeys */
/*
 * If you use anything else but MODKEY and GDK_SHIFT_MASK, don't forget to
 * edit the CLEANMASK() macro.
 */
static Key keys[] = {
    /* modifier              keyval          function    arg */
    { 0,                     GDK_KEY_s,      spawn,      SETPROP("_SURF_URI", "_SURF_GO", PROMPT_GO) },
    { 0,                     GDK_KEY_slash,  spawn,      SETPROP("_SURF_FIND", "_SURF_FIND", PROMPT_FIND) },
    { 0,                     GDK_KEY_b,      spawn,      BM_ADD("_SURF_URI") },
    { GDK_SHIFT_MASK,        GDK_KEY_b,      spawn,      BM_RM("_SURF_URI")  },

    { 0,                     GDK_KEY_f, externalpipe, { .v = linkselect_curwin } },
    { GDK_SHIFT_MASK,        GDK_KEY_f, externalpipe, { .v = linkselect_newwin } },
    { GDK_SHIFT_MASK,        GDK_KEY_u, externalpipe, { .v = editsource        } },

    { 0,                     GDK_KEY_Escape, stop,       { 0 } },

    { GDK_SHIFT_MASK,        GDK_KEY_r,      reload,     { .i = 1 } },
    { 0,                     GDK_KEY_r,      reload,     { .i = 0 } },

    { MODKEY,                GDK_KEY_i,      navigate,   { .i = +1 } },
    { MODKEY,                GDK_KEY_o,      navigate,   { .i = -1 } },

    /* vertical and horizontal scrolling, in viewport percentage */
    { 0,                     GDK_KEY_j,      scrollv,    { .i = +10 } },
    { 0,                     GDK_KEY_k,      scrollv,    { .i = -10 } },
    { 0,                     GDK_KEY_d,      scrollv,    { .i = +50 } },
    { 0,                     GDK_KEY_u,      scrollv,    { .i = -50 } },
    { 0,                     GDK_KEY_l,      scrollh,    { .i = +10 } },
    { 0,                     GDK_KEY_h,      scrollh,    { .i = -10 } },
    { 0,                     GDK_KEY_g,      scrollvmax, { .i = -100 } },
    { GDK_SHIFT_MASK,        GDK_KEY_g,      scrollvmax, { .i = +100 } },


    { GDK_SHIFT_MASK,        GDK_KEY_j,      zoom,       { .i = -1 } },
    { GDK_SHIFT_MASK,        GDK_KEY_k,      zoom,       { .i = +1 } },
    { 0,                     GDK_KEY_minus,  zoom,       { .i = -1 } },
    { 0,                     GDK_KEY_equal,  zoom,       { .i = +1 } },
    { 0,                     GDK_KEY_0,      zoom,       { .i =  0 } },

    { 0,                     GDK_KEY_p,      clipboard,  { .i = 1 } },
    { 0,                     GDK_KEY_y,      clipboard,  { .i = 0 } },
    { 0,                     GDK_KEY_w,      xdgopen,    { .v = NULL } },

    { 0,                     GDK_KEY_n,      find,       { .i = +1 } },
    { GDK_SHIFT_MASK,        GDK_KEY_n,      find,       { .i = -1 } },

    { MODKEY,                GDK_KEY_p,      print,      { 0 } },
    { MODKEY,                GDK_KEY_t,      showcert,   { 0 } },

    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_a,      togglecookiepolicy, { 0 } },
    { GDK_SHIFT_MASK,        GDK_KEY_o,      toggleinspector,    { 0 } },
    { 0,                     GDK_KEY_F12,    toggleinspector,    { 0 } },

    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_c,      toggle,     { .i = CaretBrowsing } },
    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_g,      toggle,     { .i = Geolocation } },
    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_s,      toggle,     { .i = JavaScript } },
    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_i,      toggle,     { .i = LoadImages } },
    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_v,      toggle,     { .i = Plugins } },
    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_t,      toggle,     { .i = StrictTLS } },
    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_m,      toggle,     { .i = Style } },

    /* download-console */
    { MODKEY|GDK_SHIFT_MASK, GDK_KEY_d,      spawndls,   { 0 } },

    /* quit */
    { 0,                     GDK_KEY_q,      quit,       { 0 } },
};

/* button definitions */
/* target can be OnDoc, OnLink, OnImg, OnMedia, OnEdit, OnBar, OnSel, OnAny */
static Button buttons[] = {
    /* target       event mask      button  function        argument        stop event */
    { OnLink,       0,              2,      clicknewwindow, { .i = 0 },     1 },
    { OnLink,       MODKEY,         2,      clicknewwindow, { .i = 1 },     1 },
    { OnLink,       MODKEY,         1,      clicknewwindow, { .i = 1 },     1 },
    { OnAny,        0,              8,      clicknavigate,  { .i = -1 },    1 },
    { OnAny,        0,              9,      clicknavigate,  { .i = +1 },    1 },
    { OnMedia,      MODKEY,         1,      clickexternplayer, { 0 },       1 },
};
