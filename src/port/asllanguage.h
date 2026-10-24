#ifndef ASLLANGUAGE_H
#define ASLLANGUAGE_H

// The platform's language, as ASLPlat_GetCurrentLanguage reports it (both games): it picks the
// dialog table (DIALOGFR/IT/DE/ES), the localized image and subtitle suffixes (_fr/_it/_de/_es)
// and the per-language rows of the hardcoded texts. Anything outside 1-4 is English.
enum ASLLanguage {
    ASL_LANGUAGE_ENGLISH = 0,
    ASL_LANGUAGE_FRENCH  = 1,
    ASL_LANGUAGE_ITALIAN = 2,
    ASL_LANGUAGE_GERMAN  = 3,
    ASL_LANGUAGE_SPANISH = 4,
};

int ASLPlat_GetCurrentLanguage();

#endif // ASLLANGUAGE_H
