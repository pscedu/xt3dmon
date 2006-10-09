#!/bin/sh

mencoder "mf://*.png" -o output.avi -ovc lavc -lavcopts vcodec=mpeg4:vqscale=1
