var FileSystemUtils = FileSystemUtils || {};

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
