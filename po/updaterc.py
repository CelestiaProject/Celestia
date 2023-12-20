#!/usr/bin/env python3

# Copyright © 2023, the Celestia Development Team
# Original version by Andrew Tribick
#
# Functionality based on translate_resources.pl
# © 2006-2021, the Celestia Development Team
# Original version by Christophe Teyssier <chris@teyssier.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

"""Updates a .rc file with new translations"""

from __future__ import annotations

import argparse
from gettext import GNUTranslations, NullTranslations
import os
import pathlib
from typing import Optional, TextIO, Union

from po_utils.rcfile import (
    EXTRACT_SKIP_TYPES,
    RCTokenizer,
    RC_KEYWORDS,
    Token,
    TokenType,
)
from po_utils.utilities import quote, unquote

# extracted from winnt.h/GetLocalInfo
_LANGUAGE_MAP = {
    "af": ("LANG_AFRIKAANS", "SUBLANG_NEUTRAL"),
    "af_ZA": ("LANG_AFRIKAANS", "SUBLANG_AFRIKAANS_SOUTH_AFRICA"),
    "am": ("LANG_AMHARIC", "SUBLANG_NEUTRAL"),
    "am_ET": ("LANG_AMHARIC", "SUBLANG_AMHARIC_ETHIOPIA"),
    "ar": ("LANG_ARABIC", "SUBLANG_NEUTRAL"),
    "ar_AE": ("LANG_ARABIC", "SUBLANG_ARABIC_UAE"),
    "ar_BH": ("LANG_ARABIC", "SUBLANG_ARABIC_BAHRAIN"),
    "ar_DZ": ("LANG_ARABIC", "SUBLANG_ARABIC_ALGERIA"),
    "ar_EG": ("LANG_ARABIC", "SUBLANG_ARABIC_EGYPT"),
    "ar_IQ": ("LANG_ARABIC", "SUBLANG_ARABIC_IRAQ"),
    "ar_JO": ("LANG_ARABIC", "SUBLANG_ARABIC_JORDAN"),
    "ar_KW": ("LANG_ARABIC", "SUBLANG_ARABIC_KUWAIT"),
    "ar_LB": ("LANG_ARABIC", "SUBLANG_ARABIC_LEBANON"),
    "ar_LY": ("LANG_ARABIC", "SUBLANG_ARABIC_LIBYA"),
    "ar_MA": ("LANG_ARABIC", "SUBLANG_ARABIC_MOROCCO"),
    "ar_OM": ("LANG_ARABIC", "SUBLANG_ARABIC_OMAN"),
    "ar_QA": ("LANG_ARABIC", "SUBLANG_ARABIC_QATAR"),
    "ar_SA": ("LANG_ARABIC", "SUBLANG_ARABIC_SAUDI_ARABIA"),
    "ar_SY": ("LANG_ARABIC", "SUBLANG_ARABIC_SYRIA"),
    "ar_TN": ("LANG_ARABIC", "SUBLANG_ARABIC_TUNISIA"),
    "ar_YE": ("LANG_ARABIC", "SUBLANG_ARABIC_YEMEN"),
    "arn": ("LANG_MAPUDUNGUN", "SUBLANG_NEUTRAL"),
    "arn_CL": ("LANG_MAPUDUNGUN", "SUBLANG_MAPUDUNGUN_CHILE"),
    "as": ("LANG_ASSAMESE", "SUBLANG_NEUTRAL"),
    "as_IN": ("LANG_ASSAMESE", "SUBLANG_ASSAMESE_INDIA"),
    "az": ("LANG_AZERBAIJANI", "SUBLANG_NEUTRAL"),
    "az_Cyrl_AZ": ("LANG_AZERBAIJANI", "SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC"),
    "az_Latn_AZ": ("LANG_AZERBAIJANI", "SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN"),
    "ba": ("LANG_BASHKIR", "SUBLANG_NEUTRAL"),
    "ba_RU": ("LANG_BASHKIR", "SUBLANG_BASHKIR_RUSSIA"),
    "be": ("LANG_BELARUSIAN", "SUBLANG_NEUTRAL"),
    "be_BY": ("LANG_BELARUSIAN", "SUBLANG_BELARUSIAN_BELARUS"),
    "bg": ("LANG_BULGARIAN", "SUBLANG_NEUTRAL"),
    "bg_BG": ("LANG_BULGARIAN", "SUBLANG_BULGARIAN_BULGARIA"),
    "bn": ("LANG_BENGALI", "SUBLANG_NEUTRAL"),
    "bn_BD": ("LANG_BENGALI", "SUBLANG_BENGALI_BANGLADESH"),
    "bn_IN": ("LANG_BENGALI", "SUBLANG_BENGALI_INDIA"),
    "bo": ("LANG_TIBETAN", "SUBLANG_NEUTRAL"),
    "bo_CN": ("LANG_TIBETAN", "SUBLANG_TIBETAN_PRC"),
    "br": ("LANG_BRETON", "SUBLANG_NEUTRAL"),
    "br_FR": ("LANG_BRETON", "SUBLANG_BRETON_FRANCE"),
    "bs_Cyrl_BA": ("LANG_BOSNIAN", "SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC"),
    "bs_Latn_BA": ("LANG_BOSNIAN", "SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN"),
    "ca": ("LANG_CATALAN", "SUBLANG_NEUTRAL"),
    "ca_ES": ("LANG_CATALAN", "SUBLANG_CATALAN_CATALAN"),
    "ca_ES_valencia": ("LANG_VALENCIAN", "SUBLANG_VALENCIAN_VALENCIA"),
    "chr": ("LANG_CHEROKEE", "SUBLANG_NEUTRAL"),
    "chr_Cher_US": ("LANG_CHEROKEE", "SUBLANG_CHEROKEE_CHEROKEE"),
    "co": ("LANG_CORSICAN", "SUBLANG_NEUTRAL"),
    "co_FR": ("LANG_CORSICAN", "SUBLANG_CORSICAN_FRANCE"),
    "cs": ("LANG_CZECH", "SUBLANG_NEUTRAL"),
    "cs_CZ": ("LANG_CZECH", "SUBLANG_CZECH_CZECH_REPUBLIC"),
    "cy": ("LANG_WELSH", "SUBLANG_NEUTRAL"),
    "cy_GB": ("LANG_WELSH", "SUBLANG_WELSH_UNITED_KINGDOM"),
    "da": ("LANG_DANISH", "SUBLANG_NEUTRAL"),
    "da_DK": ("LANG_DANISH", "SUBLANG_DANISH_DENMARK"),
    "de": ("LANG_GERMAN", "SUBLANG_NEUTRAL"),
    "de_AT": ("LANG_GERMAN", "SUBLANG_GERMAN_AUSTRIAN"),
    "de_CH": ("LANG_GERMAN", "SUBLANG_GERMAN_SWISS"),
    "de_DE": ("LANG_GERMAN", "SUBLANG_GERMAN"),
    "de_LI": ("LANG_GERMAN", "SUBLANG_GERMAN_LIECHTENSTEIN"),
    "de_LU": ("LANG_GERMAN", "SUBLANG_GERMAN_LUXEMBOURG"),
    "dsb_DE": ("LANG_LOWER_SORBIAN", "SUBLANG_LOWER_SORBIAN_GERMANY"),
    "dv": ("LANG_DIVEHI", "SUBLANG_NEUTRAL"),
    "dv_MV": ("LANG_DIVEHI", "SUBLANG_DIVEHI_MALDIVES"),
    "el": ("LANG_GREEK", "SUBLANG_NEUTRAL"),
    "el_GR": ("LANG_GREEK", "SUBLANG_GREEK_GREECE"),
    "en": ("LANG_ENGLISH", "SUBLANG_NEUTRAL"),
    "en_029": ("LANG_ENGLISH", "SUBLANG_ENGLISH_CARIBBEAN"),
    "en_AU": ("LANG_ENGLISH", "SUBLANG_ENGLISH_AUS"),
    "en_BZ": ("LANG_ENGLISH", "SUBLANG_ENGLISH_BELIZE"),
    "en_CA": ("LANG_ENGLISH", "SUBLANG_ENGLISH_CAN"),
    "en_GB": ("LANG_ENGLISH", "SUBLANG_ENGLISH_UK"),
    "en_IE": ("LANG_ENGLISH", "SUBLANG_ENGLISH_EIRE"),
    "en_IN": ("LANG_ENGLISH", "SUBLANG_ENGLISH_INDIA"),
    "en_JM": ("LANG_ENGLISH", "SUBLANG_ENGLISH_JAMAICA"),
    "en_MY": ("LANG_ENGLISH", "SUBLANG_ENGLISH_MALAYSIA"),
    "en_NZ": ("LANG_ENGLISH", "SUBLANG_ENGLISH_NZ"),
    "en_PH": ("LANG_ENGLISH", "SUBLANG_ENGLISH_PHILIPPINES"),
    "en_SG": ("LANG_ENGLISH", "SUBLANG_ENGLISH_SINGAPORE"),
    "en_TT": ("LANG_ENGLISH", "SUBLANG_ENGLISH_TRINIDAD"),
    "en_US": ("LANG_ENGLISH", "SUBLANG_ENGLISH_US"),
    "en_ZA": ("LANG_ENGLISH", "SUBLANG_ENGLISH_SOUTH_AFRICA"),
    "en_ZW": ("LANG_ENGLISH", "SUBLANG_ENGLISH_ZIMBABWE"),
    "es": ("LANG_SPANISH", "SUBLANG_NEUTRAL"),
    "es_AR": ("LANG_SPANISH", "SUBLANG_SPANISH_ARGENTINA"),
    "es_BO": ("LANG_SPANISH", "SUBLANG_SPANISH_BOLIVIA"),
    "es_CL": ("LANG_SPANISH", "SUBLANG_SPANISH_CHILE"),
    "es_CO": ("LANG_SPANISH", "SUBLANG_SPANISH_COLOMBIA"),
    "es_CR": ("LANG_SPANISH", "SUBLANG_SPANISH_COSTA_RICA"),
    "es_DO": ("LANG_SPANISH", "SUBLANG_SPANISH_DOMINICAN_REPUBLIC"),
    "es_EC": ("LANG_SPANISH", "SUBLANG_SPANISH_ECUADOR"),
    "es_ES": ("LANG_SPANISH", "SUBLANG_SPANISH_MODERN"),
    "es_ES_tradnl": ("LANG_SPANISH", "SUBLANG_SPANISH"),
    "es_GT": ("LANG_SPANISH", "SUBLANG_SPANISH_GUATEMALA"),
    "es_HN": ("LANG_SPANISH", "SUBLANG_SPANISH_HONDURAS"),
    "es_MX": ("LANG_SPANISH", "SUBLANG_SPANISH_MEXICAN"),
    "es_NI": ("LANG_SPANISH", "SUBLANG_SPANISH_NICARAGUA"),
    "es_PA": ("LANG_SPANISH", "SUBLANG_SPANISH_PANAMA"),
    "es_PE": ("LANG_SPANISH", "SUBLANG_SPANISH_PERU"),
    "es_PR": ("LANG_SPANISH", "SUBLANG_SPANISH_PUERTO_RICO"),
    "es_PY": ("LANG_SPANISH", "SUBLANG_SPANISH_PARAGUAY"),
    "es_SV": ("LANG_SPANISH", "SUBLANG_SPANISH_EL_SALVADOR"),
    "es_US": ("LANG_SPANISH", "SUBLANG_SPANISH_US"),
    "es_UY": ("LANG_SPANISH", "SUBLANG_SPANISH_URUGUAY"),
    "es_VE": ("LANG_SPANISH", "SUBLANG_SPANISH_VENEZUELA"),
    "et": ("LANG_ESTONIAN", "SUBLANG_NEUTRAL"),
    "et_EE": ("LANG_ESTONIAN", "SUBLANG_ESTONIAN_ESTONIA"),
    "eu": ("LANG_BASQUE", "SUBLANG_NEUTRAL"),
    "eu_ES": ("LANG_BASQUE", "SUBLANG_BASQUE_BASQUE"),
    "fa": ("LANG_PERSIAN", "SUBLANG_NEUTRAL"),
    "fa_IR": ("LANG_PERSIAN", "SUBLANG_PERSIAN_IRAN"),
    "ff": ("LANG_FULAH", "SUBLANG_NEUTRAL"),
    "ff_Latn_SN": ("LANG_FULAH", "SUBLANG_FULAH_SENEGAL"),
    "fi": ("LANG_FINNISH", "SUBLANG_NEUTRAL"),
    "fi_FI": ("LANG_FINNISH", "SUBLANG_FINNISH_FINLAND"),
    "fil": ("LANG_FILIPINO", "SUBLANG_NEUTRAL"),
    "fil_PH": ("LANG_FILIPINO", "SUBLANG_FILIPINO_PHILIPPINES"),
    "fo": ("LANG_FAEROESE", "SUBLANG_NEUTRAL"),
    "fo_FO": ("LANG_FAEROESE", "SUBLANG_FAEROESE_FAROE_ISLANDS"),
    "fr": ("LANG_FRENCH", "SUBLANG_NEUTRAL"),
    "fr_BE": ("LANG_FRENCH", "SUBLANG_FRENCH_BELGIAN"),
    "fr_CA": ("LANG_FRENCH", "SUBLANG_FRENCH_CANADIAN"),
    "fr_CH": ("LANG_FRENCH", "SUBLANG_FRENCH_SWISS"),
    "fr_FR": ("LANG_FRENCH", "SUBLANG_FRENCH"),
    "fr_LU": ("LANG_FRENCH", "SUBLANG_FRENCH_LUXEMBOURG"),
    "fr_MC": ("LANG_FRENCH", "SUBLANG_FRENCH_MONACO"),
    "fy": ("LANG_FRISIAN", "SUBLANG_NEUTRAL"),
    "fy_NL": ("LANG_FRISIAN", "SUBLANG_FRISIAN_NETHERLANDS"),
    "ga": ("LANG_IRISH", "SUBLANG_NEUTRAL"),
    "ga_IE": ("LANG_IRISH", "SUBLANG_IRISH_IRELAND"),
    "gd": ("LANG_SCOTTISH_GAELIC", "SUBLANG_NEUTRAL"),
    "gd_GB": ("LANG_SCOTTISH_GAELIC", "SUBLANG_SCOTTISH_GAELIC"),
    "gl": ("LANG_GALICIAN", "SUBLANG_NEUTRAL"),
    "gl_ES": ("LANG_GALICIAN", "SUBLANG_GALICIAN_GALICIAN"),
    "gsw": ("LANG_ALSATIAN", "SUBLANG_NEUTRAL"),
    "gsw_FR": ("LANG_ALSATIAN", "SUBLANG_ALSATIAN_FRANCE"),
    "gu": ("LANG_GUJARATI", "SUBLANG_NEUTRAL"),
    "gu_IN": ("LANG_GUJARATI", "SUBLANG_GUJARATI_INDIA"),
    "ha": ("LANG_HAUSA", "SUBLANG_NEUTRAL"),
    "ha_Latn_NG": ("LANG_HAUSA", "SUBLANG_HAUSA_NIGERIA_LATIN"),
    "haw": ("LANG_HAWAIIAN", "SUBLANG_NEUTRAL"),
    "haw_US": ("LANG_HAWAIIAN", "SUBLANG_HAWAIIAN_US"),
    "he": ("LANG_HEBREW", "SUBLANG_NEUTRAL"),
    "he_IL": ("LANG_HEBREW", "SUBLANG_HEBREW_ISRAEL"),
    "hi": ("LANG_HINDI", "SUBLANG_NEUTRAL"),
    "hi_IN": ("LANG_HINDI", "SUBLANG_HINDI_INDIA"),
    "hr": ("LANG_BOSNIAN", "SUBLANG_NEUTRAL"),
    "hr_BA": ("LANG_CROATIAN", "SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN"),
    "hr_HR": ("LANG_CROATIAN", "SUBLANG_CROATIAN_CROATIA"),
    "hsb": ("LANG_LOWER_SORBIAN", "SUBLANG_NEUTRAL"),
    "hsb_DE": ("LANG_UPPER_SORBIAN", "SUBLANG_UPPER_SORBIAN_GERMANY"),
    "hu": ("LANG_HUNGARIAN", "SUBLANG_NEUTRAL"),
    "hu_HU": ("LANG_HUNGARIAN", "SUBLANG_HUNGARIAN_HUNGARY"),
    "hy": ("LANG_ARMENIAN", "SUBLANG_NEUTRAL"),
    "hy_AM": ("LANG_ARMENIAN", "SUBLANG_ARMENIAN_ARMENIA"),
    "id": ("LANG_INDONESIAN", "SUBLANG_NEUTRAL"),
    "id_ID": ("LANG_INDONESIAN", "SUBLANG_INDONESIAN_INDONESIA"),
    "ig": ("LANG_IGBO", "SUBLANG_NEUTRAL"),
    "ig_NG": ("LANG_IGBO", "SUBLANG_IGBO_NIGERIA"),
    "ii": ("LANG_YI", "SUBLANG_NEUTRAL"),
    "ii_CN": ("LANG_YI", "SUBLANG_YI_PRC"),
    "is": ("LANG_ICELANDIC", "SUBLANG_NEUTRAL"),
    "is_IS": ("LANG_ICELANDIC", "SUBLANG_ICELANDIC_ICELAND"),
    "it": ("LANG_ITALIAN", "SUBLANG_NEUTRAL"),
    "it_CH": ("LANG_ITALIAN", "SUBLANG_ITALIAN_SWISS"),
    "it_IT": ("LANG_ITALIAN", "SUBLANG_ITALIAN"),
    "iu": ("LANG_INUKTITUT", "SUBLANG_NEUTRAL"),
    "iu_Cans_CA": ("LANG_INUKTITUT", "SUBLANG_INUKTITUT_CANADA"),
    "iu_Latn_CA": ("LANG_INUKTITUT", "SUBLANG_INUKTITUT_CANADA_LATIN"),
    "ja": ("LANG_JAPANESE", "SUBLANG_NEUTRAL"),
    "ja_JP": ("LANG_JAPANESE", "SUBLANG_JAPANESE_JAPAN"),
    "ka": ("LANG_GEORGIAN", "SUBLANG_NEUTRAL"),
    "ka_GE": ("LANG_GEORGIAN", "SUBLANG_GEORGIAN_GEORGIA"),
    "kk": ("LANG_KAZAK", "SUBLANG_NEUTRAL"),
    "kk_KZ": ("LANG_KAZAK", "SUBLANG_KAZAK_KAZAKHSTAN"),
    "kl": ("LANG_GREENLANDIC", "SUBLANG_NEUTRAL"),
    "kl_GL": ("LANG_GREENLANDIC", "SUBLANG_GREENLANDIC_GREENLAND"),
    "km": ("LANG_KHMER", "SUBLANG_NEUTRAL"),
    "km_KH": ("LANG_KHMER", "SUBLANG_KHMER_CAMBODIA"),
    "kn": ("LANG_KANNADA", "SUBLANG_NEUTRAL"),
    "kn_IN": ("LANG_KANNADA", "SUBLANG_KANNADA_INDIA"),
    "ko": ("LANG_KOREAN", "SUBLANG_NEUTRAL"),
    "ko_KR": ("LANG_KOREAN", "SUBLANG_KOREAN"),
    "kok": ("LANG_KONKANI", "SUBLANG_NEUTRAL"),
    "kok_IN": ("LANG_KONKANI", "SUBLANG_KONKANI_INDIA"),
    "ks": ("LANG_KASHMIRI", "SUBLANG_NEUTRAL"),
    "ks_Deva_IN": ("LANG_KASHMIRI", "SUBLANG_KASHMIRI_SASIA"),
    "ku": ("LANG_CENTRAL_KURDISH", "SUBLANG_NEUTRAL"),
    "ku_Arab_IQ": ("LANG_CENTRAL_KURDISH", "SUBLANG_CENTRAL_KURDISH_IRAQ"),
    "ky": ("LANG_KYRGYZ", "SUBLANG_NEUTRAL"),
    "ky_KG": ("LANG_KYRGYZ", "SUBLANG_KYRGYZ_KYRGYZSTAN"),
    "lb": ("LANG_LUXEMBOURGISH", "SUBLANG_NEUTRAL"),
    "lb_LU": ("LANG_LUXEMBOURGISH", "SUBLANG_LUXEMBOURGISH_LUXEMBOURG"),
    "lo": ("LANG_LAO", "SUBLANG_NEUTRAL"),
    "lo_LA": ("LANG_LAO", "SUBLANG_LAO_LAO"),
    "lt": ("LANG_LITHUANIAN", "SUBLANG_NEUTRAL"),
    "lt_LT": ("LANG_LITHUANIAN", "SUBLANG_LITHUANIAN"),
    "lv": ("LANG_LATVIAN", "SUBLANG_NEUTRAL"),
    "lv_LV": ("LANG_LATVIAN", "SUBLANG_LATVIAN_LATVIA"),
    "mi": ("LANG_MAORI", "SUBLANG_NEUTRAL"),
    "mi_NZ": ("LANG_MAORI", "SUBLANG_MAORI_NEW_ZEALAND"),
    "mk": ("LANG_MACEDONIAN", "SUBLANG_NEUTRAL"),
    "mk_MK": ("LANG_MACEDONIAN", "SUBLANG_MACEDONIAN_MACEDONIA"),
    "ml": ("LANG_MALAYALAM", "SUBLANG_NEUTRAL"),
    "mn": ("LANG_MONGOLIAN", "SUBLANG_NEUTRAL"),
    "mn_MN": ("LANG_MONGOLIAN", "SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA"),
    "mn_Mong_CN": ("LANG_MONGOLIAN", "SUBLANG_MONGOLIAN_PRC"),
    "mni": ("LANG_MANIPURI", "SUBLANG_NEUTRAL"),
    "moh": ("LANG_MOHAWK", "SUBLANG_NEUTRAL"),
    "moh_CA": ("LANG_MOHAWK", "SUBLANG_MOHAWK_MOHAWK"),
    "mr": ("LANG_MARATHI", "SUBLANG_NEUTRAL"),
    "mr_IN": ("LANG_MARATHI", "SUBLANG_MARATHI_INDIA"),
    "ms": ("LANG_MALAY", "SUBLANG_NEUTRAL"),
    "ms_BN": ("LANG_MALAY", "SUBLANG_MALAY_BRUNEI_DARUSSALAM"),
    "ms_MY": ("LANG_MALAY", "SUBLANG_MALAY_MALAYSIA"),
    "mt": ("LANG_MALTESE", "SUBLANG_NEUTRAL"),
    "mt_MT": ("LANG_MALTESE", "SUBLANG_MALTESE_MALTA"),
    "nb": ("LANG_NORWEGIAN", "SUBLANG_NEUTRAL"),
    "nb_NO": ("LANG_NORWEGIAN", "SUBLANG_NORWEGIAN_BOKMAL"),
    "ne": ("LANG_NEPALI", "SUBLANG_NEUTRAL"),
    "ne_IN": ("LANG_NEPALI", "SUBLANG_NEPALI_INDIA"),
    "ne_NP": ("LANG_NEPALI", "SUBLANG_NEPALI_NEPAL"),
    "nl": ("LANG_DUTCH", "SUBLANG_NEUTRAL"),
    "nl_BE": ("LANG_DUTCH", "SUBLANG_DUTCH_BELGIAN"),
    "nl_NL": ("LANG_DUTCH", "SUBLANG_DUTCH"),
    "nn_NO": ("LANG_NORWEGIAN", "SUBLANG_NORWEGIAN_NYNORSK"),
    "nso": ("LANG_SOTHO", "SUBLANG_NEUTRAL"),
    "nso_ZA": ("LANG_SOTHO", "SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA"),
    "oc": ("LANG_OCCITAN", "SUBLANG_NEUTRAL"),
    "oc_FR": ("LANG_OCCITAN", "SUBLANG_OCCITAN_FRANCE"),
    "or": ("LANG_ODIA", "SUBLANG_NEUTRAL"),
    "or_IN": ("LANG_ODIA", "SUBLANG_ODIA_INDIA"),
    "pa": ("LANG_PUNJABI", "SUBLANG_NEUTRAL"),
    "pa_Arab_PK": ("LANG_PUNJABI", "SUBLANG_PUNJABI_PAKISTAN"),
    "pa_IN": ("LANG_PUNJABI", "SUBLANG_PUNJABI_INDIA"),
    "pl": ("LANG_POLISH", "SUBLANG_NEUTRAL"),
    "pl_PL": ("LANG_POLISH", "SUBLANG_POLISH_POLAND"),
    "prs": ("LANG_DARI", "SUBLANG_NEUTRAL"),
    "prs_AF": ("LANG_DARI", "SUBLANG_DARI_AFGHANISTAN"),
    "ps": ("LANG_PASHTO", "SUBLANG_NEUTRAL"),
    "ps_AF": ("LANG_PASHTO", "SUBLANG_PASHTO_AFGHANISTAN"),
    "pt": ("LANG_PORTUGUESE", "SUBLANG_PORTUGUESE"),
    "pt_BR": ("LANG_PORTUGUESE", "SUBLANG_PORTUGUESE_BRAZILIAN"),
    "pt_PT": ("LANG_PORTUGUESE", "SUBLANG_PORTUGUESE"),
    "quc": ("LANG_KICHE", "SUBLANG_NEUTRAL"),
    "quc_Latn_GT": ("LANG_KICHE", "SUBLANG_KICHE_GUATEMALA"),
    "quz": ("LANG_QUECHUA", "SUBLANG_NEUTRAL"),
    "quz_BO": ("LANG_QUECHUA", "SUBLANG_QUECHUA_BOLIVIA"),
    "quz_EC": ("LANG_QUECHUA", "SUBLANG_QUECHUA_ECUADOR"),
    "quz_PE": ("LANG_QUECHUA", "SUBLANG_QUECHUA_PERU"),
    "rm": ("LANG_ROMANSH", "SUBLANG_NEUTRAL"),
    "rm_CH": ("LANG_ROMANSH", "SUBLANG_ROMANSH_SWITZERLAND"),
    "ro": ("LANG_ROMANIAN", "SUBLANG_NEUTRAL"),
    "ro_RO": ("LANG_ROMANIAN", "SUBLANG_ROMANIAN_ROMANIA"),
    "ru": ("LANG_RUSSIAN", "SUBLANG_NEUTRAL"),
    "ru_RU": ("LANG_RUSSIAN", "SUBLANG_RUSSIAN_RUSSIA"),
    "rw": ("LANG_KINYARWANDA", "SUBLANG_NEUTRAL"),
    "rw_RW": ("LANG_KINYARWANDA", "SUBLANG_KINYARWANDA_RWANDA"),
    "sa": ("LANG_SANSKRIT", "SUBLANG_NEUTRAL"),
    "sa_IN": ("LANG_SANSKRIT", "SUBLANG_SANSKRIT_INDIA"),
    "sah": ("LANG_SAKHA", "SUBLANG_NEUTRAL"),
    "sah_RU": ("LANG_SAKHA", "SUBLANG_SAKHA_RUSSIA"),
    "sd": ("LANG_SINDHI", "SUBLANG_NEUTRAL"),
    "sd_Arab_PK": ("LANG_SINDHI", "SUBLANG_SINDHI_PAKISTAN"),
    "sd_Deva_IN": ("LANG_SINDHI", "SUBLANG_SINDHI_INDIA"),
    "se": ("LANG_SAMI", "SUBLANG_NEUTRAL"),
    "se_FI": ("LANG_SAMI", "SUBLANG_SAMI_NORTHERN_FINLAND"),
    "se_NO": ("LANG_SAMI", "SUBLANG_SAMI_NORTHERN_NORWAY"),
    "se_SE": ("LANG_SAMI", "SUBLANG_SAMI_NORTHERN_SWEDEN"),
    "si": ("LANG_SINHALESE", "SUBLANG_NEUTRAL"),
    "si_LK": ("LANG_SINHALESE", "SUBLANG_SINHALESE_SRI_LANKA"),
    "sk": ("LANG_SLOVAK", "SUBLANG_NEUTRAL"),
    "sk_SK": ("LANG_SLOVAK", "SUBLANG_SLOVAK_SLOVAKIA"),
    "sl": ("LANG_SLOVENIAN", "SUBLANG_NEUTRAL"),
    "sl_SI": ("LANG_SLOVENIAN", "SUBLANG_SLOVENIAN_SLOVENIA"),
    "sma_NO": ("LANG_SAMI", "SUBLANG_SAMI_SOUTHERN_NORWAY"),
    "sma_SE": ("LANG_SAMI", "SUBLANG_SAMI_SOUTHERN_SWEDEN"),
    "smj_NO": ("LANG_SAMI", "SUBLANG_SAMI_LULE_NORWAY"),
    "smj_SE": ("LANG_SAMI", "SUBLANG_SAMI_LULE_SWEDEN"),
    "smn_FI": ("LANG_SAMI", "SUBLANG_SAMI_INARI_FINLAND"),
    "sms_FI": ("LANG_SAMI", "SUBLANG_SAMI_SKOLT_FINLAND"),
    "sq": ("LANG_ALBANIAN", "SUBLANG_NEUTRAL"),
    "sq_AL": ("LANG_ALBANIAN", "SUBLANG_ALBANIAN_ALBANIA"),
    "sr_Cyrl_BA": ("LANG_SERBIAN", "SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC"),
    "sr_Cyrl_CS": ("LANG_SERBIAN", "SUBLANG_SERBIAN_CYRILLIC"),
    "sr_Cyrl_ME": ("LANG_SERBIAN", "SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC"),
    "sr_Cyrl_RS": ("LANG_SERBIAN", "SUBLANG_SERBIAN_SERBIA_CYRILLIC"),
    "sr_Latn_BA": ("LANG_SERBIAN", "SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN"),
    "sr_Latn_CS": ("LANG_SERBIAN", "SUBLANG_SERBIAN_LATIN"),
    "sr_Latn_ME": ("LANG_SERBIAN", "SUBLANG_SERBIAN_MONTENEGRO_LATIN"),
    "sr_Latn_RS": ("LANG_SERBIAN", "SUBLANG_SERBIAN_SERBIA_LATIN"),
    "sv": ("LANG_SWEDISH", "SUBLANG_NEUTRAL"),
    "sv_FI": ("LANG_SWEDISH", "SUBLANG_SWEDISH_FINLAND"),
    "sv_SE": ("LANG_SWEDISH", "SUBLANG_SWEDISH"),
    "sw": ("LANG_SWAHILI", "SUBLANG_NEUTRAL"),
    "sw_KE": ("LANG_SWAHILI", "SUBLANG_SWAHILI_KENYA"),
    "syr": ("LANG_SYRIAC", "SUBLANG_NEUTRAL"),
    "syr_SY": ("LANG_SYRIAC", "SUBLANG_SYRIAC_SYRIA"),
    "ta": ("LANG_TAMIL", "SUBLANG_NEUTRAL"),
    "ta_IN": ("LANG_TAMIL", "SUBLANG_TAMIL_INDIA"),
    "ta_LK": ("LANG_TAMIL", "SUBLANG_TAMIL_SRI_LANKA"),
    "te": ("LANG_TELUGU", "SUBLANG_NEUTRAL"),
    "te_IN": ("LANG_TELUGU", "SUBLANG_TELUGU_INDIA"),
    "tg": ("LANG_TAJIK", "SUBLANG_NEUTRAL"),
    "tg_Cyrl_TJ": ("LANG_TAJIK", "SUBLANG_TAJIK_TAJIKISTAN"),
    "th": ("LANG_THAI", "SUBLANG_NEUTRAL"),
    "th_TH": ("LANG_THAI", "SUBLANG_THAI_THAILAND"),
    "ti": ("LANG_TIGRINYA", "SUBLANG_NEUTRAL"),
    "ti_ER": ("LANG_TIGRINYA", "SUBLANG_TIGRINYA_ERITREA"),
    "ti_ET": ("LANG_TIGRINYA", "SUBLANG_TIGRINYA_ETHIOPIA"),
    "tk": ("LANG_TURKMEN", "SUBLANG_NEUTRAL"),
    "tk_TM": ("LANG_TURKMEN", "SUBLANG_TURKMEN_TURKMENISTAN"),
    "tn": ("LANG_TSWANA", "SUBLANG_NEUTRAL"),
    "tn_BW": ("LANG_TSWANA", "SUBLANG_TSWANA_BOTSWANA"),
    "tn_ZA": ("LANG_TSWANA", "SUBLANG_TSWANA_SOUTH_AFRICA"),
    "tr": ("LANG_TURKISH", "SUBLANG_NEUTRAL"),
    "tr_TR": ("LANG_TURKISH", "SUBLANG_TURKISH_TURKEY"),
    "tt": ("LANG_TATAR", "SUBLANG_NEUTRAL"),
    "tt_RU": ("LANG_TATAR", "SUBLANG_TATAR_RUSSIA"),
    "tzm": ("LANG_TAMAZIGHT", "SUBLANG_NEUTRAL"),
    "tzm_Latn_DZ": ("LANG_TAMAZIGHT", "SUBLANG_TAMAZIGHT_ALGERIA_LATIN"),
    "tzm_Tfng_MA": ("LANG_TAMAZIGHT", "SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH"),
    "ug": ("LANG_UIGHUR", "SUBLANG_NEUTRAL"),
    "ug_CN": ("LANG_UIGHUR", "SUBLANG_UIGHUR_PRC"),
    "uk": ("LANG_UKRAINIAN", "SUBLANG_NEUTRAL"),
    "uk_UA": ("LANG_UKRAINIAN", "SUBLANG_UKRAINIAN_UKRAINE"),
    "ur": ("LANG_URDU", "SUBLANG_NEUTRAL"),
    "ur_IN": ("LANG_URDU", "SUBLANG_URDU_INDIA"),
    "ur_PK": ("LANG_URDU", "SUBLANG_URDU_PAKISTAN"),
    "uz": ("LANG_UZBEK", "SUBLANG_NEUTRAL"),
    "uz_Cyrl_UZ": ("LANG_UZBEK", "SUBLANG_UZBEK_CYRILLIC"),
    "uz_Latn_UZ": ("LANG_UZBEK", "SUBLANG_UZBEK_LATIN"),
    "vi": ("LANG_VIETNAMESE", "SUBLANG_NEUTRAL"),
    "vi_VN": ("LANG_VIETNAMESE", "SUBLANG_VIETNAMESE_VIETNAM"),
    "wo": ("LANG_WOLOF", "SUBLANG_NEUTRAL"),
    "wo_SN": ("LANG_WOLOF", "SUBLANG_WOLOF_SENEGAL"),
    "xh": ("LANG_XHOSA", "SUBLANG_NEUTRAL"),
    "xh_ZA": ("LANG_XHOSA", "SUBLANG_XHOSA_SOUTH_AFRICA"),
    "yo": ("LANG_YORUBA", "SUBLANG_NEUTRAL"),
    "yo_NG": ("LANG_YORUBA", "SUBLANG_YORUBA_NIGERIA"),
    "zh": ("LANG_CHINESE", "SUBLANG_NEUTRAL"),
    "zh_CN": ("LANG_CHINESE", "SUBLANG_CHINESE_SIMPLIFIED"),
    "zh_HK": ("LANG_CHINESE", "SUBLANG_CHINESE_HONGKONG"),
    "zh_MO": ("LANG_CHINESE", "SUBLANG_CHINESE_MACAU"),
    "zh_SG": ("LANG_CHINESE", "SUBLANG_CHINESE_SINGAPORE"),
    "zh_TW": ("LANG_CHINESE", "SUBLANG_CHINESE_TRADITIONAL"),
    "zu": ("LANG_ZULU", "SUBLANG_NEUTRAL"),
    "zu_ZA": ("LANG_ZULU", "SUBLANG_ZULU_SOUTH_AFRICA"),
}

_SOURCE_LANG = "LANG_ENGLISH"
_SOURCE_SUBLANG = "SUBLANG_ENGLISH_US"


def _write_language(
    tokenizer: RCTokenizer, out_file: TextIO, lang: str, sublang: str
) -> None:
    state = 0
    tokens: list[Token] = []
    while True:
        try:
            token = next(tokenizer)
        except StopIteration:
            break

        if token.type in EXTRACT_SKIP_TYPES:
            tokens.append(token)
            continue

        if (
            state == 0
            and token.type == TokenType.KEYWORD
            and token.value == _SOURCE_LANG
        ):
            tokens.append(Token(TokenType.KEYWORD, lang, token.line))
            state = 1
        elif state == 1 and token.type == TokenType.OPERATOR and token.value == ",":
            tokens.append(token)
            state = 2
        elif (
            state == 2
            and token.type == TokenType.KEYWORD
            and token.value == _SOURCE_SUBLANG
        ):
            tokens.append(Token(TokenType.KEYWORD, sublang, token.line))
            break
        else:
            tokens.append(token)
            break

    for token in tokens:
        out_file.write(token.value)


def _process_nc(
    tokenizer: RCTokenizer,
    messages: NullTranslations,
) -> list[Token]:
    state = 0
    tokens: list[Token] = []
    message: Optional[str] = None
    context: Optional[str] = None
    while True:
        try:
            token = next(tokenizer)
        except StopIteration:
            break

        tokens.append(token)
        if token.type in EXTRACT_SKIP_TYPES:
            continue

        if state == 0 and token.type == TokenType.OPEN and token.value == "(":
            state = 1
        elif state == 1 and token.type == TokenType.QUOTED:
            context = unquote(token.value)
            state = 2
        elif state == 2 and token.type == TokenType.OPERATOR and token.value == ",":
            state = 3
        elif state == 3 and token.type == TokenType.QUOTED:
            message = unquote(token.value)
            state = 4
        elif state == 4 and token.type == TokenType.CLOSE and token.value == ")":
            new_message = messages.pgettext(context, message)
            tokens = [Token(TokenType.QUOTED, quote(new_message), token.line)]
            break
        else:
            break

    return tokens


def _write_keyword(
    tokenizer: RCTokenizer,
    out_file: TextIO,
    messages: NullTranslations,
) -> None:
    tokens: list[Token] = []
    while True:
        try:
            token = next(tokenizer)
        except StopIteration:
            break

        if token.type in EXTRACT_SKIP_TYPES:
            tokens.append(token)
            continue

        if token.type == TokenType.QUOTED:
            message = unquote(token.value)
            if message.strip():
                new_message = messages.gettext(message)
                tokens.append(Token(TokenType.QUOTED, quote(new_message), token.line))
            else:
                tokens.append(token)
        elif token.type == TokenType.KEYWORD and token.value == "NC_":
            tokens += _process_nc(tokenizer, messages)
        else:
            tokens.append(token)
        break

    for token in tokens:
        out_file.write(token.value)


def _write_translated(
    tokenizer: RCTokenizer, out_file: TextIO, messages: NullTranslations, lang_tag: str
) -> None:
    lang, sublang = _LANGUAGE_MAP[lang_tag]
    while True:
        try:
            token = next(tokenizer)
        except StopIteration:
            break
        out_file.write(token.value)
        if token.type != TokenType.KEYWORD:
            continue
        if token.value == "LANGUAGE":
            _write_language(tokenizer, out_file, lang, sublang)
        elif token.value in RC_KEYWORDS:
            _write_keyword(tokenizer, out_file, messages)


def update_rc(
    rc_path: Union[str, os.PathLike],
    mo_path: Union[str, os.PathLike],
    out_path: Union[None, str, os.PathLike],
) -> None:
    """Creates a translated .rc file."""
    lang_tag = os.path.splitext(os.path.basename(mo_path))[0]
    if not out_path:
        parts = os.path.splitext(rc_path)
        out_path = f"{parts[0]}_{lang_tag}{parts[1]}"
    with open(mo_path, "rb") as mo_file:
        messages = GNUTranslations(mo_file)
    with open(rc_path, "rt", encoding="utf-16") as rc_file:
        tokenizer = RCTokenizer(rc_file)
        with open(out_path, "wt", encoding="utf-16") as out_file:
            _write_translated(tokenizer, out_file, messages, lang_tag)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Update RC file with translations")
    parser.add_argument(
        "rcfile",
        type=pathlib.Path,
        help="Resource file to update",
    )
    parser.add_argument(
        "mofile",
        type=pathlib.Path,
        help=".mo file containing translations",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="output",
        type=pathlib.Path,
        help="Output file",
        required=False,
    )

    args = parser.parse_args()
    update_rc(args.rcfile, args.mofile, args.output)
