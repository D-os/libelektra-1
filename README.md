= About =

Elektra provides a universal and secure framework to store configuration
parameters in a global, hierarchical key database.  The core is a small
library implemented in C. The plugin-based framework fulfills many
configuration-related tasks to avoid any unnecessary code duplication
across applications while it still allows the core to stay without any
external dependency. Elektra abstracts from cross-platform-related issues
with an consistent API, and allows applications to be aware of other
applications' configurations, leveraging easy application integration.

== Facts and Features ==

 * Elektra implements an API to fully access a global key database.
 * Elektra supports mounting of existing configuration files into the global key database.
 * Elektra is multi-process safe and can be used in multi-threaded programs.
 * Elektra (except for some plugins) is portable and completely written in Ansi-C99.
 * Elektra (except for some plugins) has no external dependency.
 * Elektra uses the [BSD licence](doc/COPYING).
 * Elektra is suitable for embedded systems and early boot stage programs.
 * Elektra supports comments and other non-configuration information by meta data.
 * Elektra can import, export and convert supported configuration files.
 * Elektra is able to log and notify other software on any configuration changes using [[Software/dbus|dbus]].
 * Elektra is able to avoid the problem that any invalid configuration is written into the permanent storage.
 * Elektra is able to provide different mechanisms to locate configuration files.
 * Elektra supports different ways to escape and encode content of configuration files.
 * Standard key/value pair hierarchy and semantics are defined within freedesktop.org.


== Further Information ==

To get an introduction, it is best to take a look at the
[presentation](http://www.libelektra.org/ftp/elektra/presentations/2012/lgm.odp),
see the
[poster](http://www.libelektra.org/ftp/elektra/poster.pdf)
and read the
[abridgment](http://www.libelektra.org/ftp/elektra/abridgement.pdf).

The currently best information about Elektra is
[this thesis](http://www.libelektra.org/ftp/elektra/thesis.pdf).

The API documentation can be found
(here)[http://doc.libelektra.org/api/current/html].



== Contact ==

Do not hesitate to ask any question on

 (Mailing List)[https://lists.sourceforge.net/lists/listinfo/registry-list]

or one of the

 [[doc/AUTHORS]].


= Get Started =

== Download ==

Elektra's uses a (git repository at github)[https://github.com/ElektraInitiative/libelektra].

Releases can be downloaded from
[[ftp://ftp.libelektra.org/elektra/releases/|ftp]]
and
[[http://www.libelektra.org/ftp/elektra/releases/|http]]

Which are also [[http://gitorious.org/elektra-initiative/ftp|mirrored at]]:

 git-clone git://gitorious.org/elektra-initiative/ftp.git

To use the debian repository of the releases put following files in
/etc/apt/sources.list:
{{{
deb     http://build.libelektra.org/debian/ elektra-release-glue main
deb-src http://build.libelektra.org/debian/ elektra-release-glue main
}}}

The [[http://build.libelektra.org:8080/|build server]] builds release
and master branches on every commit.


== Compile ==

See the
[[doc/COMPILE]]
document in the repository for informations how to
compile the software.


== Install ==

The preferred way to install Elektra is by using packages provided for
your distribution.

If there are no packages available, see the
[[https://gitorious.org/elektra-initiative/libelektra/blobs/master/doc/INSTALL|INSTALL]]
document.