#!/bin/zsh

if [[ -e $1 ]]; then
	[[ -w $1 ]] && sed -e 's/[[:blank:]]*#[[:blank:]]*IgnoreGLExtensions/ IgnoreGLExtensions/' -i .bak $1;
else
	echo 'Configuration {\nIgnoreGLExtensions [ "GL_ARB_vertex_program" ]\n}' > $1;
fi
