#!/bin/bash

python3 link_emulation.py run -d $FORWARD_LINK $FORWARD_TRACE $RETURN_LINK $RETURN_TRACE >> /tmp/trace.log 2>&1
