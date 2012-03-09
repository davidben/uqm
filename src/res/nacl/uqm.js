var handleProgress = (function () {
    var downloadsDiv = document.getElementById("downloads");
    var downloadsProgress = null;
    var downloads = {};
    function handleProgress(path, bytes, size) {
	if (size >= 0 && bytes >= size) {
	    console.log(path + " done");
	    if (downloads[path]) {
		delete downloads[path];
	    }
	} else {
	    if (downloads[path] === undefined)
		console.log("Downloading '" + path + "'...");
	    downloads[path] = [bytes, size]
	}
	updateProgress();
    }
    function updateProgress() {
	var numDownloads = 0;
	var totalDownloaded = 0, totalSize = 0;
	for (var path in downloads) {
	    if (!downloads.hasOwnProperty(path))
		continue;
	    numDownloads++;
	    if (downloads[path][1] > 0) {
		totalDownloaded += downloads[path][0];
		totalSize += downloads[path][1];
	    }
	}

	if (numDownloads == 0) {
	    // No active downloads. Delete the progress bar.
	    if (downloadsProgress) {
		downloadsProgress.parentElement.removeChild(downloadsProgress);
		downloadsProgress = null;
	    }
	    return;
	}
	// Create a progress bar and update it.
	if (!downloadsProgress) {
	    downloadsProgress = document.createElement("progress");
	    downloadsDiv.appendChild(downloadsProgress);
	}
	if (totalSize > 0) {
	    downloadsProgress.value = totalDownloaded;
	    downloadsProgress.max = totalSize;
	} else {
	    // Assume indeterminate.
	    downloadsProgress.max = null;
	}
    }

    return handleProgress;
}());

function initializeModule(fsPersistent) {
    // Plumb nexe loading code through handleProgress for now. All
    // this needs better UI anyway.
    var containerDiv = document.getElementById("container");
    containerDiv.addEventListener("loadstart", function (ev) {
	handleProgress("Downloading", -1, -1);
    }, true);
    containerDiv.addEventListener("progress", function (ev) {
	if (ev.lengthComputable) {
	    handleProgress("Downloading", ev.loaded, ev.total);
	} else {
	    handleProgress("Downloading", -1, -1);
	}
    }, true);
    containerDiv.addEventListener("load", function (ev) {
	handleProgress("Downloading", 1, 1);
	handleProgress("Initializing", -1, -1);
    }, true);
    containerDiv.addEventListener("loadend", function (ev) {
	handleProgress("Initializing", 1, 1);
    }, true);

    var uqmModule = document.createElement("embed");
    uqmModule.setAttribute("width", "640");
    uqmModule.setAttribute("height", "480");
    uqmModule.setAttribute("manifest", manifest);
    uqmModule.setAttribute("manifestsize", manifestSize);
    uqmModule.setAttribute("src", "uqm.nmf");
    uqmModule.setAttribute("type", "application/x-nacl");
    containerDiv.appendChild(uqmModule);
    uqmModule.tabIndex = 1;
    uqmModule.focus();

    function readDirectory(path) {
	function onError(e) {
            var msg = FileSystemUtils.errorToString(e.code);
            console.log("Error reading '" + path + "': "
			+ msg + " (" + e.code + ")");
	    uqmModule.postMessage(["ReadDirectory", "ERROR"].join("\x00"));
	}

	fsPersistent.root.getDirectory(path, {}, function (dirEntry) {
	    FileSystemUtils.readDirectory(
		dirEntry, function (entries) {
		    uqmModule.postMessage(
			["ReadDirectory", "OK"].concat(entries).join("\x00"));
		}, onError);
	}, onError);
    }

    uqmModule.addEventListener(
	"message", function (ev) {
	    var msg = ev.data.split("\x00");
	    if (msg[0] == "ReadDirectory") {
		readDirectory(msg[1]);
	    } else if (msg[0] == "HandleProgress") {
		handleProgress(msg[1], Number(msg[2]), Number(msg[3]));
	    } else {
		console.log("Unexpected message:");
		console.log(msg);
	    }
	}, false);
}

(function () {
    function onError(e) {
        var msg = FileSystemUtils.errorToString(e.code);
        console.log(msg + " (" + e.code + ")");
    }

    // Request 2MB storage for save data. This should be way more than
    // enough space.
    window.webkitStorageInfo.requestQuota(
	PERSISTENT, 2 * 1024 * 1024, function (grantedBytes) {
	    window.webkitRequestFileSystem(
		PERSISTENT, grantedBytes, function (fsPersistent) {
		    console.log("Got filesystem. Starting game.");
		    initializeModule(fsPersistent);
		}, onError);
	    }, function (e) { console.log(e); });
}());
