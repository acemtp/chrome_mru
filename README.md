Introduction
============

`chrome_mru` is a small (<50kb) Windows application that run in the trailbar.

Its goal is to enable you to use CTRL+TAB in Google Chrome with the Most Recently Used Tab behavior (and not the default next/previous tab).

Installation
============

* [Download](https://github.com/downloads/acemtp/chrome_mru/chrome_mru.exe) the Windows binary (or recompile it with Visual Studio C++ Express 2008)
* There's no install, it's just a standalone program, put it somewhere (c:\bin) and double click on it
* Install the [Recent Tabs](https://chrome.google.com/webstore/detail/ocllfmhjhfmogablefmibmjcodggknml) Google Chrome extension
* Go to the `Recent Tabs` settings and set the `Shortcut key` to `CLTR+~` (the key above the `Tab` key)

How it works
============

It's a very dumb program I coded in a few hours. It detects if the focused windows contains "Google Chrome" in the title and if yes,
`chrome_mru` catch `CTRL+TAB` key before Google Chrome and instead generates a `CTRL+~` key so it calls the `Recent Tabs` extension to switch in the MRU behavior.

It works fine for me but it really needs more iteration to be great. If some people find it useful, I'll spend more time on it. All contributions are welcome!
