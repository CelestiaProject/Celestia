#!/usr/bin/perl

# Extract translatable strings from a windows resource files 
# and outputs them in a C like format for use by gettext.

use strict;

# List of keywords with a string to translate
my %keywords = (
    'POPUP' => 1,
    'MENUITEM' => 1,
    'DEFPUSHBUTTON' => 1,
    'LTEXT' => 1,
    'CAPTION' => 1,
    'PUSHBUTTON' => 1,
    'RTEXT' => 1,
    'CTEXT' => 1,
    'LTEXT' => 1,
    'GROUPBOX' => 1,
    'CONTROL' => 1
);

while (<>) {
    # Matches lines:
    #   KEYWORD  "String" ...
    if ( $_ =~ /^\s*(\w+)\s+"(.+?)"/ ) { 
        # Excludes blank strings and strings of the form word followed by digit
        # which are control names (e.g. DateTimePicker3)
        if (exists($keywords{$1}) && $2 !~ /^ *$/ && $2 !~ /^\w+\d$/ ) {
            print qq{_("$2");\n};
        }
    }
}
print qq{_("WinLangID");
    _("Jan"), _("Feb"), _("Mar"), _("Apr"), _("May"), _("Jun"),
    _("Jul"), _("Aug"), _("Sep"), _("Oct"), _("Nov"), _("Dec")
};
