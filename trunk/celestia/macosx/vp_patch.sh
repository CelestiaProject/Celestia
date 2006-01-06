#!/bin/zsh

[[ -w $1 ]] && sed -e 's/^#[[:blank:]]*IgnoreGLExtensions/IgnoreGLExtensions/' -i .bak $1
