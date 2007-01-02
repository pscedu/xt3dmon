#!/bin/sh

mencoder "mf://*.png" -o output.avi -mf fps=30 -ovc lavc -lavcopts vcodec=mpeg4:vqscale=1
