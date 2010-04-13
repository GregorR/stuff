<html><head>
<title>Conference Manager</title>
<?PHP
$db = new SQLite3("conferences.db");

// if this is a request, add the conferences
if (isset($_REQUEST["name"])) {
    $submdate = str_replace(",", " ", $_REQUEST["submdate"]);
    $confdate = str_replace(",", " ", $_REQUEST["confdate"]);

    $sql = "DELETE FROM conferences WHERE name = \"" . $db->escapeString($_REQUEST["name"]) . "\" AND " .
        "year = " . intval($_REQUEST["year"]) . ";";
    $db->query($sql)->finalize();

    $sql = "INSERT INTO conferences (name, year, submdate, confdate, confloc, estimated) VALUES (" .
        "\"" . $db->escapeString($_REQUEST["name"]) . "\", " .
        intval($_REQUEST["year"]) . ", " .
        strtotime($submdate) . ", " .
        strtotime($confdate) . ", " .
        "\"" . $db->escapeString($_REQUEST["confloc"]) . "\", " .
        (isset($_REQUEST["estimated"]) ? "1" : "0") . ");";
    $db->query($sql)->finalize();
}
?>

<script type="text/javascript">
var confbysubmdate = [];
var confbyconfdate = [];

var confbyname = ({
<?PHP
// get all the conferences
$sql = "SELECT * FROM conferences ORDER BY submdate ASC;";
$res = $db->query($sql);

while (($row = $res->fetchArray()) !== false) {
    $cname = $row["name"] . " " . $row["year"];
    $rmonyr = strftime("%B, %Y", $row["submdate"]);
    $rdt = strftime("%d %B, %Y", $row["submdate"]);

    print "\"" . $cname . "\": {" .
        "cname:\"" . addslashes($cname) . "\"," .
        "name1:\"" . addslashes($row["name"]) . "\"," .
        "year:" . $row["year"] . "," .

        "submtime:" . $row["submdate"] . "," .
        "submday:\"" . strftime("%d", $row["submdate"]) . "\"," .
        "submmon:\"" . strftime("%B", $row["submdate"]) . "\"," .
        "submyr:" . strftime("%Y", $row["submdate"]) . "," .

        "conftime:" . $row["confdate"] . "," .
        "confday:\"" . strftime("%d", $row["confdate"]) . "\"," .
        "confmon:\"" . strftime("%B", $row["confdate"]) . "\"," .
        "confyr:" . strftime("%Y", $row["confdate"]) . "," .

        "estimated:" . ($row["estimated"] ? "true" : "false") . "," .
        "confloc:\"" . addslashes($row["confloc"]) . "\"" .

        "},";
}

$res->finalize();
?>
na:0});
delete confbyname.na;

// now fill in new (estimated) values
var curdt = new Date();
var curtm = Math.floor(curdt.getTime() / 1000);
var curyr = curdt.getFullYear();
var maxyr = curyr + 2;
var minyr = curyr - 2;
function fillConfs() {
    var changed = true;

    while (changed) {
        changed = false;

        for (var conf in confbyname) {
            conf = confbyname[conf];

            // if there isn't one for the next year ...
            if (conf.year > minyr && conf.year < maxyr) {
                var cnextyr = conf.name1 + " " + (conf.year + 1);
                if (typeof(confbyname[cnextyr]) == "undefined") {
                    // invent it
                    confbyname[cnextyr] = ({
                        cname: cnextyr,
                        name1: conf.name1,
                        year: conf.year + 1,
                        
                        submtime: conf.submtime + (60 * 60 * 24 * 365),
                        submday: conf.submday,
                        submmon: conf.submmon,
                        submyr: conf.submyr + 1,
                        
                        conftime: conf.conftime + (60 * 60 * 24 * 365),
                        confday: conf.confday,
                        confmon: conf.confmon,
                        confyr: conf.confyr + 1,
                        
                        estimated: true,
                        confloc: "?"
                    });
                    changed = true;
                }
            }
        }
    }
}
fillConfs();

for (var conf in confbyname) {
    conf = confbyname[conf];
    confbysubmdate[conf.submtime] = conf;
    confbyconfdate[conf.conftime] = conf;
}

function ce(type) {
    return document.createElement(type);
}

function clear(elem) {
    if (elem.hasChildNodes()) {
        while (elem.childNodes.length >= 1) {
            elem.removeChild(elem.children[0]);
        }
    }
}

function bySomething(sorted, name) {
    var div = document.getElementById("confman");
    var monyr = "";

    clear(div);

    // get our array of dates
    var dates = [];
    for (var i in sorted) {
        dates.push(i);
    }
    dates = dates.sort();

    // build it up
    for (var i = 0; i < dates.length; i++) {
        var conf = sorted[dates[i]];

        if (conf.conftime < curtm) continue;

        // make a header
        var cmonyr = conf[name + "mon"] + " " + conf[name + "yr"];
        if (cmonyr != monyr) {
            div.appendChild(ce("hr"));
            var h2 = ce("h2");
            h2.innerHTML = cmonyr;
            div.appendChild(h2);
            monyr = cmonyr;
        }

        // add this conference
        var a = ce("a");
        a.href = "http://google.com/search?q=" + conf.cname;
        a.innerHTML = conf.cname;
        div.appendChild(a);

        var p = ce("span");
        var html = " " + conf.submday + " " + conf.submmon + ", " + conf.submyr + "; " +
            conf.confday + " " + conf.confmon + ", " + conf.confyr + "; " +
            conf.confloc + "; " +
            "<a href=\"#addform\" onclick=\"fillConf('" + conf.cname + "');\">";

        if (conf.estimated) {
            html += "(estimated from previous year)";
        } else {
            html += "(correct)";
        }

        html += "</a>";
        p.innerHTML = html;

        div.appendChild(p);
        div.appendChild(ce("br"));
    }
}

function bySubmDate() {
    bySomething(confbysubmdate, "subm");
}

function byConfDate() {
    bySomething(confbyconfdate, "conf");
}

function gebn(name) {
    return document.getElementsByName(name)[0];
}

function fillConf(name) {
    var conf = confbyname[name];

    // fill in the form
    gebn("name").value = conf.name1;
    gebn("year").value = conf.year;
    gebn("submdate").value = conf.submday + " " + conf.submmon + ", " + conf.submyr;
    gebn("confdate").value = conf.confday + " " + conf.confmon + ", " + conf.confyr;
    gebn("confloc").value = conf.confloc;
    gebn("estimated").checked = conf.estimated;
}
</script>

</head><body onload="bySubmDate();">
<h1>Conference Manager</h1>
<div id="confman"></div>

<hr/>
<input type="button" value="By submission date" onclick="bySubmDate();" />
<input type="button" value="By conference date" onclick="byConfDate();" />

<hr/>
<h2>Add</h2>
<a name="addform"></a>
<form action="confman.php" method="post">
<table border=0>
<tr><td>Conference name</td>        <td><input type="text" name="name" /></td></tr>
<tr><td>Year</td>                   <td><input type="text" name="year" /></td></tr>
<tr><td>Submission date</td>        <td><input type="text" name="submdate" /></td></tr>
<tr><td>Conference start date</td>  <td><input type="text" name="confdate" /></td></tr>
<tr><td>Conference location</td>    <td><input type="text" name="confloc" /></td></tr>
<tr><td>Estimated</td>              <td><input type="checkbox" name="estimated" /></td></tr>
</table>
<input type="submit" value="Add" />
</form>

</body></html>
