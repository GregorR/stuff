function updateImage(p) {
    var pimg = document.getElementById("p" + p);
    pimg.src = "dinosaurLoader.gif";

    function performUpdate() {
        var url = "dinosaurComic.php?panels=" + p + "&r=" + Math.random();
        pimg.src = url;
    }
    setTimeout(performUpdate, 0);
}

function hideImage(p) {
    var pimg = document.getElementById("p" + p);
    var curvis = pimg.style.visibility;
    if (curvis == "visible" || curvis == "show" || curvis == "") {
        pimg.style.visibility = "hidden";
    } else if (curvis == "hidden" || curvis == "hide") {
        pimg.style.visibility = "visible";
    }
}
