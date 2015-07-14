<?php
/*
 * Copyright 2004 Matt Brubeck
 * Richard Ash 2006
 * Gale Andrews 2009-2015
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  require_once "../legacy/versions.inc.php";
  require_once "../legacy/mirror.inc.php";  
  $pageId = "mac";
  $pageTitle = _("Legacy Mac OS 9 / X");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<div class="advice">
<?=_('<b>These are legacy downloads. Please use the </b> <a href="../download/mac">latest Audacity version</a><b> for Mac OS X 10.5 to 10.10.x.</b>')?>
</div>

<h3><?=_("Download")?></h3>
<p><a href="#sysreq"><?=_("System Requirements")?></a></p>
<ul>
  <li><p><?=_('For Mac OS X 10.4 (<b>Universal binary</b>): <a href="http://www.oldfoss.com/Audacity/download/audacity-macosx-ub-2.0.6.dmg">Audacity 2.0.6</a> (.dmg file, 30.9 MB) - use this also on PPC Macs on OS X 10.5 if you want to use Audio Units in Audacity')?></p></li>
  <li><p><?php printf(_('For Mac OS X 10.0 to 10.3 (<b>Intel</b>): <a href="%s">Audacity %s</a> (.dmg file, %.1lf MB)'), download_url($macosx_intel_url), macosx_intel_version, macosx_intel_size)?></p></li>
  <li><p><?php printf(_('For Mac OS X 10.0 to 10.3 (<b>PPC</b>): <a href="%s">Audacity %s</a> (.dmg file, %.1lf MB)'), download_url ($macosx_url), macosx_version, macosx_size)?></p></li>
  <li><p><?php printf(_('For Mac OS 9: <a href="%s">Audacity %s</a> (.sit file, %.1lf MB)'), download_url($mac_classic_url), mac_classic_version, mac_classic_size)?></p></li>
  <p><?=_("(Unfortunately, we no longer have the resources to provide new versions of Audacity for Mac OS 9.)")?></p></li>
</ul>

<p> <b><?=_("Installation instructions (OS X .dmg files)")?>:</b>
 <ol>
   <li><?=_("Inside your Applications folder, create a folder called \"Audacity\"")?></li>
   <li><?=_("Double-click the downloaded .dmg to mount it")?></li>
   <li><?=_("Option-drag the whole of the .dmg contents (not the .dmg itself) into the \"Audacity\" folder you created")?></li>
   <li><?=_("Double-click the Audacity icon inside the \"Audacity\" folder to launch the program")?></li>
 </ol>
</p>

<h3 id="optional"><?=_("Optional Downloads")?></h3>

<h4><?=_("Plug-ins and Libraries")?></h4>
<ul>
  <li><p><a href="http://ardour.org/files/releases/swh-plugins-0.4.15.dmg"><?=_("LADSPA plug-ins installer</a> - over 90 plug-ins.")?></p></li>
  <li><p><a href="plugins"><?=_("Plug-Ins")?></a> - <?=_("Download additional effects and filters.")?></p></li>
  <li><p><a href="../help/faq?s=install&amp;item=lame-mp3"><?=_("LAME MP3 encoder")?></a> - <?=_("Allows Audacity to export MP3 files.")?></p></li>
</ul>

<h4><?=_("Alternative Download Links")?></h4>    
<ul>
  <li>
    <p><?php printf(_('If you have trouble with your download, or need an older version of Audacity, try:')) ?></p>
    <ul>
      <li><?php printf(_('<a href="%s">OldFoss</a>: View older versions by clicking to enter the required folder under "Version".'), "http://www.oldfoss.com/Audacity.html")?></li>
      <li><?php printf(_('<a href="%s">Google Code</a>: Click on the headings to sort the list.'), "http://code.google.com/p/audacity/downloads/list")?></li>
    </ul>
  </li>
</ul>

<h3 id="sysreq"><?=_("System Requirements")?></h3>
<?=_('The <a href="../download/mac">latest Audacity version</a> requires Mac OS X 10.5 or later')?>
<p>
<ul>
<li><?=_("Audacity 2.0.6 is a legacy version for Mac OS X 10.4 (Universal binary) only.")?></li>
<li><?=_("Audacity 1.2.5 is a legacy version for Mac OS X 10.0 to 10.3 (Intel) only.")?></li>
<li><?=_("Audacity 1.2.6a is a legacy version for Mac OS X 10.0 to 10.3 (PPC) only.")?></li>
<li><?=_("Audacity 1.0.0 is a legacy version for Mac OS 9 only.")?></li>
</ul>
</p>

<p>
<div class="advice">
<b><?=_("Audacity runs best with at least 1 GB RAM and a 1 GHz processor (2 GB RAM/2 GHz on OS X 10.7 or later).")?></b> 
<p><?=_("Where Audacity is to be used for lengthy multi-track projects, we recommend a minimum of 2 GB RAM and 2 GHz processor (4 GB RAM on OS X 10.7 or later).")?></p>
</div>
</p>

<?php
  include "../include/footer.inc.php";
?>
