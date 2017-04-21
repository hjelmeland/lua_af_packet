# af_packet 
af_packet - Minimal Lua binding to AF_PACKET, SOCK_RAW sockets. 

This makes it possible to send and receive raw ethernet frames using Lua.

The binding is luaposix style, using integer file handlers instead of
file handler objects. The code is based on code extracted from luaposix.

Only tested with Lua 5.1.

## Usage:

	local p = require 'af_packet'
	local ETH_P_HPAV = 0x88E1 -- Sample ether type : HomePlug AV 
	local fh,err = p.socket(p.AF_PACKET, p.SOCK_RAW, ETH_P_HPAV)
	assert(fh,err)

	p.bindif(fh, 'eth0') -- bind to interface


	local ethframe = string.char(...)

	local res,err = p.send(fh,ethframe)
	assert(res,err)

	local max_frame_len=1000
	local ethframe, err = p.recv(fh, max_frame_len)
	assert(ethframe, err)

	p.close(fh)

