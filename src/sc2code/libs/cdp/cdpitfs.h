/* TODO: review/comment [inline]/change rules */

/*
 * CDP Custom Interface KINDs repository
 *
 * Officially registered interfaces live here. The rules are:
 * 1) anything up to 65536 is reserved for internal use
 * 2) please (!) apply for your KIND (range) officially in UQM
 *    bugzilla (for now)
 * 3) your choices for KIND names are either your email addy
 *    or your mod project name, followed by a brief but descriptive
 *    interface name; please keep is short, the final identifier
 *    has to be under 64 chars long
 * 4) upon application, you get a KIND numerical range for your
 *    use; please use range sizes that are a multiple of 0x10
 *    (self managed for now), but do not take more than 0x40
 *    at once
 *
 * Sample:
 *
 * CDPITF_KIND_CODEPRO_AT_USA_NET_HANGAR       = 0x00010001,
 * CDPITF_KIND_CODEPRO_AT_USA_NET_IMAGEEFFECTS = 0x00010002,
 */
