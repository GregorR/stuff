<?PHP
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
        if ($i < 0 || $i > $panels) die("Invalid panel.");
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

// pick random comics
$comics = array();
while (count($comics) < $panels) {
    $comic = mt_rand(1, $latest);
    if ($comic < 10) $comic = "0" . $comic; else $comic = "" . $comic;

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
}

// now start producing our output
$outimg = imagecreatetruecolor($ow, $oh);

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

// and write out a png
header("Content-type: image/png");
imagepng($outimg);
imagedestroy($outimg);
?>
