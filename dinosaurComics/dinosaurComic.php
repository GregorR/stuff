<?PHP
/*
 * Copyright (C) 2009 Gregor Richards
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

$latest = 1538;
$panels = 7; // 6 plus the footer
$w = 735;
$h = 500;
$border = 3;
$ow = $w;
$oh = $h;
$xs = array(
    array(0, 246),
    array(246, 375),
    array(375, 735),
    array(0, 196),
    array(196, 494),
    array(494, 735),
    array(0, 735));
$ys = array(
    array(0, 244),
    array(0, 244),
    array(0, 244),
    array(244, 487),
    array(244, 487),
    array(244, 487),
    array(487, 500));
$adjx = 0;
$adjy = 0;
$same = false;

$includePanels = array(0, 1, 2, 3, 4, 5, 6);

// optionally choose a subset of panels
if (isset($_REQUEST["panels"])) {
    $rpanels = explode(",", $_REQUEST["panels"]);
    $includePanels = array();
    foreach ($rpanels as $panel) $includePanels[] = intval($panel);

    // get the adjustment
    $maxx = $maxy = 0;
    $minx = $w;
    $miny = $h;
    foreach ($includePanels as $i) {
        if ($i < 0 || $i >= $panels) die("Invalid panel.");
        if ($xs[$i][0] < $minx) $minx = $xs[$i][0];
        if ($xs[$i][1] > $maxx) $maxx = $xs[$i][1];
        if ($ys[$i][0] < $miny) $miny = $ys[$i][0];
        if ($ys[$i][1] > $maxy) $maxy = $ys[$i][1];
    }
    if ($minx > 0) $minx -= $border;
    if ($miny > 0) $miny -= $border;

    $adjx = $minx;
    $adjy = $miny;
    $ow = $maxx - $minx;
    $oh = $maxy - $miny;
}
if (count($includePanels) > $panels) die("Too many panels!");
$panels = count($includePanels);

// potentially use supplied comics
$incomics = array();
if (isset($_REQUEST["comics"])) {
    $incomicsprime = explode(",", $_REQUEST["comics"]);
    $incomics = array();
    foreach ($incomicsprime as $ic) $incomics[] = intval($ic);
}

// or be less random
if (isset($_REQUEST["same"])) {
    $same = true;
}

// pick random comics
$comics = array();
$comicstrs = array();
while (count($comics) < $panels) {
    if (count($incomics) > 0) {
        $comic = array_shift($incomics);
    } else if ($same && count($comics) > 0) {
        $comic = $comicstrs[0];
    } else {
        $comic = mt_rand(1, $latest);
        if ($comic < 10) $comic = "0" . $comic; else $comic = "" . $comic;
    }

    // get the filename ...
    $fn = "comic2-" . $comic . ".png";
    $cfn = "cache/" . $fn;

    // download it ...
    if (!file_exists($cfn)) {
        continue;
        touch($cfn) || die();

        // open it ...
        $fh = @fopen("http://www.qwantz.com/comics/" . $fn, "r");
        if ($fh === false) continue;

        // read it in ...
        $cpng = stream_get_contents($fh);
        fclose($fh);

        // write it out
        file_put_contents($cfn, $cpng) || die();

        if (strlen($cpng) > 0) {
            $inimg = imagecreatefromstring($cpng);
            if ($inimg === false) continue;
        } else {
            continue;
        }

    } else if (filesize($cfn) == 0) {
        continue;

    } else {
        $inimg = imagecreatefrompng($cfn);
        if ($inimg === false) continue;

    }

    // verify that the file is usable
    if (imagesx($inimg) != $w || imagesy($inimg) != $h) {
        imagedestroy($inimg);
        continue;
    }

    $comics[] = $inimg;
    $comicstrs[] = $comic;
}

// make the permalink
$comicstr = "?";
if (isset($_REQUEST["panels"])) {
    $comicstr .= "panels=" . $_REQUEST["panels"] . "&";
}
$comicstr .= "comics=" . implode(",", $comicstrs);

// now start producing our output
$outimg = imagecreatetruecolor($ow, $oh + imagefontheight(1) + 4);

// read in the comic images
for ($i = 0; $i < $panels; $i++) {
    // read in this panel
    $inimg = $comics[$i];
    $p = $includePanels[$i];

    // then copy it over
    imagecopy($outimg, $inimg,
              $xs[$p][0] - $adjx, $ys[$p][0] - $adjy, $xs[$p][0], $ys[$p][0],
              ($xs[$p][1] - $xs[$p][0]), ($ys[$p][1] - $ys[$p][0]));
    imagedestroy($inimg);
}

// write the comicstr
$white = imagecolorallocate($outimg, 255, 255, 255);
$grey = imagecolorallocate($outimg, 128, 128, 128);
imagefilledrectangle($outimg, 0, $oh, imagesx($outimg), imagesy($outimg), $white);
imagestring($outimg, 1, 2, $oh + 2, $comicstr, $grey);

// and write out a png
header("Content-type: image/png");
imagepng($outimg);
imagedestroy($outimg);
?>
