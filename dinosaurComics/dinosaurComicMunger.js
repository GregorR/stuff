if( typeof XMLHttpRequest == "undefined" ) XMLHttpRequest = function() {
  try { return new ActiveXObject("Msxml2.XMLHTTP.6.0") } catch(e) {}
  try { return new ActiveXObject("Msxml2.XMLHTTP.3.0") } catch(e) {}
  try { return new ActiveXObject("Msxml2.XMLHTTP") } catch(e) {}
  try { return new ActiveXObject("Microsoft.XMLHTTP") } catch(e) {}
  throw new Error( "This browser does not support XMLHttpRequest." )
};

var panelComics = [0, 0, 0, 0, 0, 0];
var panelVisible = [true, true, true, true, true, true];

function updateImage(p) {
    var pimg = document.getElementById("p" + p);
    pimg.src = "dinosaurLoader.gif";

    var request = new XMLHttpRequest();
    var url = "dinosaurComic.php?panels=" + p + "&json";
    request.open("GET", url, true);

    function onrsc() {
        if (request.readyState == 4 && request.status == 200 && request.responseText) {
            var result = eval("("+request.responseText+")");
            pimg.src = "data:image/png;base64," + result.comic;
            panelComics[p] = result.panels[0];
            updateDisplay();
        }
    }
    request.onreadystatechange = onrsc;
    request.send("");
}

function updateImages() {
    var i;
    for (i = 0; i < 6; i++) {
        updateImage(i);
    }
}

function hideImage(p) {
    var pimg = document.getElementById("p" + p);
    var curvis = pimg.style.visibility;
    if (curvis == "visible" || curvis == "show" || curvis == "") {
        pimg.style.visibility = "hidden";
        panelVisible[p] = false;
    } else if (curvis == "hidden" || curvis == "hide") {
        pimg.style.visibility = "visible";
        panelVisible[p] = true;
    }
    updateDisplay();
}

function updateDisplay() {
    var url = "dinosaurComic.php?panels=";
    var i;

    // visibility
    for (i = 0; i < 6; i++) {
        if (panelVisible[i]) {
            url += i + ",";
        }
    }
    url = url.slice(0, -1);

    // numbers
    url += "&comics=";
    for (i = 0; i < 6; i++) {
        if (panelVisible[i]) {
            url += panelComics[i] + ",";
        }
    }
    url = url.slice(0, -1).replace("&", "&amp;");

    document.getElementById("permalink").innerHTML =
        "<a href=\"" + url + "\">" + url + "</a>";
}
