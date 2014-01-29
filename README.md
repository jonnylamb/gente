gente
=====

A tiny GNOME app that reads a name to timezone key file and shows
where in the world and what the current time is for said person. It
was heaviily based on the timezone editor in gnome-control-center.

There is a sample key file installed into `$prefix/share/gente/data.txt`
but if you create a `~/.config/gente/data.txt` it will be read instead.

Data file
---------

The data file must be called `data.txt` and must be "key file" format
with one group called "people". For example:

```
[people]
Jacques Chirac=Europe/Paris
Silvio Berlusconi=Europe/Rome
Bob Hawke=Australia/Sydney
```

A data file is shipped and installed, into `$prefix/share/gente/data.txt`.
This is just for testing as its contents are a joke. You can use
your own key file by putting it in `~/.config/gente/` ensuring it too
is called `data.txt`. This will override the installed test data file.

Timezone names
--------------

The names of the timezones are read from
`/usr/share/zoneinfo/zone.tab`. When adding people to the data file
ensure to check the timezone exists in this file or it won't show up
in the map correctly. If a timezone is not recognised, the following
warning will appear on the command line when opening the program:

``** (gente:29821): WARNING **: Unsupported timezone 'Europe/Barcelona' for 'Ramon Salazar'``

gnome-control-center components
-------------------------------

Most of the `src/` directory was lifted from gnome-control-center's
`panels/datetime/` directory:

* cc-timezone.map.[ch]: the GTK widget used for displaying the world
  map and its timezones.
* data/: the images to show the world map accurately.
* timedate1-interface.xml: XML introspection data for the
  org.freedesktop.timedate1 D-Bus service.
* tz.[ch]: timezone utilities originally from Anaconda for dealing
  with known quirks with timezone.
