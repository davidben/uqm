var FileSystemUtils = FileSystemUtils || {};

FileSystemUtils.TaskManager = function (doneCallback) {
    this.pending = 0;
    this.doneCallback = doneCallback;
};
FileSystemUtils.TaskManager.prototype = {
    push: function () { this.pending++; },
    pop: function () { if (--this.pending == 0) this.doneCallback(); }
};

FileSystemUtils.errorToString = function(code) {
    switch (code) {
    case FileError.QUOTA_EXCEEDED_ERR:
	return 'QUOTA_EXCEEDED_ERR';
    case FileError.NOT_FOUND_ERR:
	return 'NOT_FOUND_ERR';
    case FileError.SECURITY_ERR:
	return 'SECURITY_ERR';
    case FileError.INVALID_MODIFICATION_ERR:
	return 'INVALID_MODIFICATION_ERR';
    case FileError.INVALID_STATE_ERR:
	return 'INVALID_STATE_ERR';
    default:
	return 'Unknown Error';
    };
};

/** Reads a directory entry into a list. Calls successCallback on an
 * Array on success. */
FileSystemUtils.readDirectory = function (dirEntry,
					  successCallback,
					  errorCallback) {
    var reader = dirEntry.createReader();
    var entries = [];

    // This API is kinda ridiculous.
    function pumpReader() {
	reader.readEntries(function (results) {
	    if (results.length == 0) {
		successCallback(entries);
	    } else {
		for (var i = 0; i < results.length; i++) {
		    entries.push(results[i].name);
		}
		pumpReader();
	    }
	}, errorCallback);
    }
    pumpReader();
};

/** Copy a file cross-filesystems. */
FileSystemUtils.xfsCopyFile = function (fileEntry, dirEntry, newName,
					successCallback, errorCallback) {
    console.log("Copying file " + fileEntry.name);
    fileEntry.file(function (file) {
	dirEntry.getFile(
	    newName, {create: true, exclusive: true}, function (destEntry) {
		destEntry.createWriter(function (fileWriter) {
		    fileWriter.onwriteend = function (e) {
			successCallback(destEntry);
		    };
		    fileWriter.onerror = errorCallback;
		    fileWriter.write(file);
		});
	    }, errorCallback);
    }, errorCallback);
};

/** Recursively a directory cross-filesystems. */
FileSystemUtils.xfsCopyDirectory = function (srcDir, destDir, newName,
					     successCallback, errorCallback) {
    console.log("Copying directory " + srcDir.name);
    destDir.getDirectory(
	newName, {create: true, exclusive: true}, function (destEntry) {
	    var tasks = new FileSystemUtils.TaskManager(function () {
		successCallback(destEntry);
	    });

	    var dirReader = srcDir.createReader();
	    function loop() {
		tasks.push();
		dirReader.readEntries(function (results) {
		    if (results.length == 0) {
			// Done.
			tasks.pop();
			return;
		    }
		    // Schedule a batch of copies.
		    for (var i = 0; i < results.length; i++) {
			tasks.push();
			if (results[i].isFile)
			    FileSystemUtils.xfsCopyFile(
				results[i], destEntry, results[i].name,
				tasks.pop.bind(tasks), errorCallback);
			else
			    FileSystemUtils.xfsCopyDirectory(
				results[i], destEntry, results[i].name,
				tasks.pop.bind(tasks), errorCallback);
		    }
		    // And loop again.
		    loop();
		    tasks.pop();
		}, errorCallback);
	    }
	    loop();
	}, errorCallback);
};

/** getDirectory, but returns null instead of error if not found. */
FileSystemUtils.getDirectoryOrNull = function (dirEntry, path,
					       successCallback, errorCallback) {
    dirEntry.getDirectory(
	path, {}, function (entry) { successCallback(entry); },
	function (e) {
	    if (e.code == FileError.NOT_FOUND_ERR) {
		successCallback(null);
	    } else {
		errorCallback(e);
	    }
	});
};
