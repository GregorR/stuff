function updateImage(p) {
    var pimg = document.getElementById("p" + p);
    pimg.src = "dinosaurLoader.gif";
    var url = "dinosaurComic.php?panels=" + p + "&r=" + Math.random();
    pimg.src = url;
}

function hideImage(p) {
    document.getElementById("p" + p).style.visibility = "hidden";
}
