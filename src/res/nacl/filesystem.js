var FileSystemUtils = FileSystemUtils || {};

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
}

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
