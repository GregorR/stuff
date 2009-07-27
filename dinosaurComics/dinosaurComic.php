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

// figure out the latest Dinosaur Comic
$dlf = "cache/dinosaur-latest";
if (!file_exists($dlf) || time() - filemtime($dlf) > (60 * 60 * 24)) {
    // get the latest comic
    $fh = fopen("http://www.qwantz.com/index.php", "r");
    if ($fh === false) die("Failed to connect to qwantz.com!");

    $page = str_replace("\n", " ", stream_get_contents($fh));
    fclose($fh);

    // now extract the comic
    $num = preg_replace("/.*comic2-([0-9]*)\\.png.*/", "\\1", $page);
    file_put_contents($dlf, $num);
}

$latest = intval(file_get_contents($dlf));
$panels = 7; // 6 plus the footer
$w = 735;
$h = 500;
$border = 3;
$ow = $w;
$oh = $h;
$xs = array(
    array(0, 245),
    array(242, 375),
    array(372, 735),
    array(0, 196),
    array(193, 494),
    array(491, 735),
    array(0, 735));
$ys = array(
    array(0, 244),
    array(0, 244),
    array(0, 244),
    array(242, 487),
    array(242, 487),
    array(242, 487),
    array(487, 500));
$adjx = 0;
$adjy = 0;
$same = false;
$strip = false;

$includePanels = array(0, 1, 2, 3, 4, 5, 6);

// perhaps use JSON output
$useJSON = false;
if (isset($_REQUEST["json"])) {
    $useJSON = true;
}

// optionally choose a subset of panels
if (isset($_REQUEST["panels"])) {
    if (isset($_REQUEST["strip"])) {
        // create a strip, rather than a comic with holes
        $strip = true;
    }

    $rpanels = explode(",", $_REQUEST["panels"]);
    $includePanels = array();
    foreach ($rpanels as $panel) $includePanels[] = intval($panel);

    // get the adjustment
    if (!$strip) {
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
    } else {
        $minx = $miny = $maxy = 0;
        $maxx = $border;
        foreach ($includePanels as $i) {
            if ($i < 0 || $i >= $panels) die("Invalid panel.");
            $maxx += $xs[$i][1] - $xs[$i][0] - $border;
            if ($ys[$i][1] - $ys[$i][0] > $maxy) $maxy = $ys[$i][1] - $ys[$i][0];
        }
    }

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
    $comic = 0;
    if (count($incomics) > 0) {
        $comic = array_shift($incomics);
    } else if ($same && count($comics) > 0) {
        $comic = $comicstrs[0];
    }
    
    if ($comic == 0) {
        $comic = mt_rand(1, $latest);
    }
    if (gettype($comic) == "integer") {
        if ($comic < 0 || $comic > $latest) die("Invalid comic $comic.");
        if ($comic < 10) $comic = "0" . $comic; else $comic = "" . $comic;
    }

    // get the filename ...
    $fn = "comic2-" . $comic . ".png";
    $cfn = "cache/" . $fn;

    // download it ...
    if (!file_exists($cfn)) {
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
if ($strip) {
    $comicstr .= "&strip";
}
$panelstr = implode(",", $comicstrs);

// check if we're resizing
$resize = 1;
if (isset($_REQUEST["resize"])) {
    $resize = floatval($_REQUEST["resize"]);
    if ($resize <= 0) $resize = 0.1;
    if ($resize > 2) $resize = 2;
    $ow *= $resize;
    $oh *= $resize;
}

// now start producing our output
$outimg = imagecreatetruecolor($ow, $oh + imagefontheight(1) + 4);

// read in the comic images
$curx = 0;
for ($i = 0; $i < $panels; $i++) {
    // read in this panel
    $inimg = $comics[$i];
    $p = $includePanels[$i];

    // figure out where to put it
    $fx = $xs[$p][0];
    $fy = $ys[$p][0];
    $fw = $xs[$p][1] - $xs[$p][0];
    $fh = $ys[$p][1] - $ys[$p][0];
    $tw = floor($fw * $resize);
    $th = floor($fh * $resize);
    if (!$strip) {
        $tx = floor(($fx - $adjx) * $resize);
        $ty = floor(($fy - $adjy) * $resize);
    } else {
        $tx = floor($curx);
        $ty = 0;
        $curx += $tw - ($border * $resize);
    }

    // then copy it over
    if ($resize === 1) {
        imagecopy($outimg, $inimg,
                  $tx, $ty, $fx, $fy, $fw, $fh);
    } else {
        imagecopyresampled($outimg, $inimg,
                  $tx, $ty, $fx, $fy, $tw, $th, $fw, $fh);
    }
    imagedestroy($inimg);
}

// write the comicstr
$white = imagecolorallocate($outimg, 255, 255, 255);
$grey = imagecolorallocate($outimg, 128, 128, 128);
imagefilledrectangle($outimg, 0, $oh, imagesx($outimg), imagesy($outimg), $white);
imagestring($outimg, 1, 2, $oh + 2, $comicstr, $grey);

// and write out a png
if (!$useJSON) {
    header("Content-type: image/png");
    imagepng($outimg);
} else {
    //header("Content-type: application/json");

    ob_start();
    imagepng($outimg);
    $imgpng = ob_get_contents();
    ob_end_clean();

    print "{\"permalink\": \"" . $comicstr . "\", \"panels\": [" . $panelstr . "], \"comic\": \"" . base64_encode($imgpng) . "\"}";
}
imagedestroy($outimg);
?>
