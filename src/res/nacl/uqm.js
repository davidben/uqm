(function () {
    var downloadsList = document.getElementById("downloads");
    var downloads = {}
    function HandleProgress(path, bytes, size) {
	if (size >= 0 && bytes >= size) {
	    console.log(path + " done");
	    if (downloads[path]) {
		downloads[path].parentElement.removeChild(downloads[path]);
		delete downloads[path];
	    }
	} else {
	    if (!downloads[path]) {
		var li = document.createElement('li');
		li.appendChild(document.createTextNode(path));
		li.appendChild(document.createElement('progress'));
		downloadsList.appendChild(li);
		downloads[path] = li;
	    }
	    if (size >= 0) {
		var progress = downloads[path].lastChild;
		progress.max = size;
		progress.value = bytes;
	    }
	}
    }

    // Plumb nexe loading code through HandleProgress for now. All
    // this needs better UI anyway.
    var containerDiv = document.getElementById("container");
    containerDiv.addEventListener("loadstart", function (ev) {
	HandleProgress("Downloading", -1, -1);
    }, true);
    containerDiv.addEventListener("progress", function (ev) {
	if (ev.lengthComputable) {
	    HandleProgress("Downloading", ev.loaded, ev.total);
	} else {
	    HandleProgress("Downloading", -1, -1);
	}
    }, true);
    containerDiv.addEventListener("load", function (ev) {
	HandleProgress("Downloading", 1, 1);
	HandleProgress("Initializing", -1, -1);
    }, true);
    containerDiv.addEventListener("loadend", function (ev) {
	HandleProgress("Initializing", 1, 1);
    }, true);

    var uqmModule = document.createElement("embed");
    uqmModule.setAttribute("width", "640");
    uqmModule.setAttribute("height", "480");
    uqmModule.setAttribute("manifest", manifest);
    uqmModule.setAttribute("manifestsize", manifestSize);
    uqmModule.setAttribute("src", "uqm.nmf");
    uqmModule.setAttribute("type", "application/x-nacl");
    containerDiv.appendChild(uqmModule);

    function ReadDirectory(path) {
	function onError(e) {
            var msg = '';

            switch (e.code) {
	    case FileError.QUOTA_EXCEEDED_ERR:
		msg = 'QUOTA_EXCEEDED_ERR';
		break;
	    case FileError.NOT_FOUND_ERR:
		msg = 'NOT_FOUND_ERR';
		break;
	    case FileError.SECURITY_ERR:
		msg = 'SECURITY_ERR';
		break;
	    case FileError.INVALID_MODIFICATION_ERR:
		msg = 'INVALID_MODIFICATION_ERR';
		break;
	    case FileError.INVALID_STATE_ERR:
		msg = 'INVALID_STATE_ERR';
		break;
	    default:
		msg = 'Unknown Error';
		break;
	    };

            console.log("Error reading '" + path + "': "
			+ msg + " (" + e + ")");
	    uqmModule.postMessage(["ReadDirectory", "ERROR"].join("\x00"));
	}

	// This API is kinda ridiculous.
	function readDirectory(dirEntry) {
	    var reader = dirEntry.createReader();
	    var entries = [];

	    function done() {
		uqmModule.postMessage(
		    ["ReadDirectory", "OK"].concat(entries).join("\x00"));
	    }
	    function pumpReader() {
		reader.readEntries(function (results) {
		    if (results.length == 0) {
			done();
		    } else {
			for (var i = 0; i < results.length; i++) {
			    entries.push(results[i].name);
			}
			pumpReader();
		    }
		}, onError);
	     }
	     pumpReader();
	}

	window.webkitRequestFileSystem(
            window.TEMPORARY, 5*1024*1024,
            function (fs) {
		fs.root.getDirectory(path, {}, readDirectory, onError);
	    }, onError);
    }


    uqmModule.addEventListener(
	"message", function (ev) {
	    var msg = ev.data.split("\x00");
	    if (msg[0] == "ReadDirectory") {
		ReadDirectory(msg[1]);
	    } else if (msg[0] == "HandleProgress") {
		HandleProgress(msg[1], Number(msg[2]), Number(msg[3]));
	    } else {
		console.log("Unexpected message:");
		console.log(msg);
	    }
	}, false);
})();
