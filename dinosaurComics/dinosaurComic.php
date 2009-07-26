<?PHP
$latest = 1538;
$panels = 7; // 6 plus the footer
$w = 735;
$h = 500;
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
        $fh = fopen("http://www.qwantz.com/comics/" . $fn, "r");
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
    print "Trying $cfn ... " . imagesx($inimg) . "x" . imagesy($inimg) . "<br/>";
    if (imagesx($inimg) != $w || imagesy($inimg) != $h) {
        imagedestroy($inimg);
        continue;
    }

    $comics[] = $inimg;
}

// now start producing our output
$outimg = imagecreatetruecolor($w, $h);

// read in the comic images
for ($i = 0; $i < $panels; $i++) {
    // read in this panel
    $inimg = $comics[$i];

    // then copy it over
    imagecopy($outimg, $inimg,
              $xs[$i][0], $ys[$i][0], $xs[$i][0], $ys[$i][0],
              ($xs[$i][1] - $xs[$i][0]), ($ys[$i][1] - $ys[$i][0]));
    imagedestroy($inimg);
}

// and write out a png
header("Content-type: image/png");
imagepng($outimg);
imagedestroy($outimg);
?>
