<?php
/*
 * Copyright 2004 Matt Brubeck
 * Copyright 2005 Dominic Mazzoni
 * 2008 - 2015 Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  $pageId = "linux";
  $pageTitle = _("GNU/Linux");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<p><?=_('Installation packages for Audacity on GNU/Linux and other Unix-like systems are provided by individual distributions. The following distributions maintain regularly updated Audacity packages:')?></p>

<p><?=_('
<ul>
 <li><a href="http://koji.fedoraproject.org/koji/packageinfo?packageID=1352">Fedora</a></li>
 <li><a href="http://packages.gentoo.org/package/media-sound/audacity">Gentoo</a></li>
 <li><A href="http://packman.links2linux.org/package/286">SuSE/Open SUSE</a></li>
 <li>Ubuntu:
 <ul><li><a href="http://packages.ubuntu.com/search?keywords=audacity&searchon=names&suite=all&section=all">Ubuntu release packages.</a> <b>Note:</b> Ubuntu 2.0.6 releases are built against an <a href="http://wiki.audacityteam.org/wiki/Incorrect_wxWidgets_Version">incorrect version of wxWidgets</a>.</li>
     <li><a href="https://launchpad.net/~ubuntuhandbook1/+archive/ubuntu/audacity">Audacity 2.1.0 release PPA package</a> for Ubuntu 12.04, 14.04, 14.10 and 15.04 (also Linux Mint 13 and 17). This is built against the correct wx2.8.12. Uninstall any packaged version of Audacity before installing the PPA.</li>
     <li><a href="http://rpmseek.com/rpm-pl/audacity.html?hl=com&cx=0%3A-%3A0%3A0%3A0%3A0%3A0&qArStr=0&qRtStr=0&qDnStr=109">rpmseek</a> for legacy Audacity packages.</li>
</ul></ul>
')?></p>

<p><?=_('Otherwise, we recommend either searching the web pages of your distribution for the latest information, or compiling Audacity yourself from <a href="source">source code</a>.')?></p>

<h3 id="sysreq"><?=_("System Requirements")?></h3>
<?=_("We recommend using the latest version of GNU/Linux from your distribution that is compatible with your hardware specifications. Audacity will run best with at least 64 MB RAM and a 300 MHz processor.")?></li>


<?php
  include "../include/footer.inc.php";
?>
