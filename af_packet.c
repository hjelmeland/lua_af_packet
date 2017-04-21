#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include "lua.h"
#include "lauxlib.h"

#define VERSION "0.00"


static int pusherror(lua_State *L, const char *info)
{
	lua_pushnil(L);
	if (info==NULL)
		lua_pushstring(L, strerror(errno));
	else
		lua_pushfstring(L, "%s: %s", info, strerror(errno));
	lua_pushinteger(L, errno);
	return 3;
}

static int pushresult(lua_State *L, int i, const char *info)
{
	if (i==-1)
		return pusherror(L, info);
	lua_pushinteger(L, i);
	return 1;
}

/***
Create an endpoint for communication.
@function socket
@see socket(2)
@int domain one of AF\_INET, AF\_INET6, AF\_UNIX or AF\_NETLINK
@int type one of SOCK\_STREAM, SOCK\_DGRAM or SOCK\_RAW
@int options usually 0, but some socket types might imlement other protocols.
@treturn int socket descriptor if successful, nil otherwise
@treturn string error message if failed
@usage sockd = posix.socket (posix.AF_INET, posix.SOCK_STREAM, 0)
*/
static int Psocket(lua_State *L)
{
	int domain = luaL_checknumber(L, 1);
	int type = luaL_checknumber(L, 2);
	int protocol = luaL_checknumber(L, 3);
	return pushresult(L, socket(domain, type, htons(protocol)), "socket");
}


static int Pbindif(lua_State *L)
{
	int fd = luaL_checknumber(L, 1);
	const char * iface = luaL_optstring (L,  2, "eth0");
	int protocol       = luaL_optinteger (L, 3, 0);

	struct sockaddr_ll ll;
	unsigned int if_index = if_nametoindex(iface);
	memset(&ll, 0, sizeof(ll));
	ll.sll_family = AF_PACKET;
	ll.sll_ifindex = if_index;
	ll.sll_protocol = htons(protocol);

	return pushresult(L, bind(fd, (struct sockaddr *)&ll, sizeof(ll)), "bind");
}


/***
Send a message from a socket.
@function send
@see send(2)
@int fd socket descriptor
@string buffer message bytes to send
@return number of bytes sent if successful, otherwise nil
@return error message if failed
*/
static int Psend(lua_State *L)
{
	int fd = luaL_checknumber(L, 1);
	size_t len;
	const char *buf = luaL_checklstring(L, 2, &len);

	return pushresult(L, send(fd, buf, len, 0), NULL);
}



/***
Receive a message from a socket.
@function recv
@see recv(2)
@int fd socket descriptor
@int count number of bytes to receive
@return received bytes if successful, otherwise nil
@return error message if failed
*/
static int Precv(lua_State *L)
{
	int fd = luaL_checkint(L, 1);
	int count = luaL_checkint(L, 2), ret;
	void *ud, *buf;
	lua_Alloc lalloc = lua_getallocf(L, &ud);

	/* Reset errno in case lalloc doesn't set it */
	errno = 0;
	if ((buf = lalloc(ud, NULL, 0, count)) == NULL && count > 0)
		return pusherror(L, "lalloc");

	ret = recv(fd, buf, count, 0);
	if (ret < 0) {
		lalloc(ud, buf, count, 0);
		return pusherror(L, NULL);
	}

	lua_pushlstring(L, buf, ret);
	lalloc(ud, buf, count, 0);
	return 1;
}



/***
Close an open file descriptor.
@function close
@see close(2)
@int fd
@return 0 on success, nil otherwise
@return error message if failed.
*/
static int Pclose(lua_State *L)
{
	int fd = luaL_checkint(L, 1);
	return pushresult(L, close(fd), NULL);
}



#define MENTRY(_s) { (_s), (_s)}

static const luaL_reg Raf_packet[] = {
	{"socket",   Psocket},
	{"send",   Psend},
	{"recv",   Precv},
	{"bindif",   Pbindif},
	{"close",    Pclose},
	{NULL, NULL}
};

LUALIB_API int luaopen_af_packet(lua_State *L) {
	
	lua_newtable (L);
	luaL_register(L, NULL, Raf_packet);


	lua_pushliteral(L, VERSION);
	lua_setfield(L, -2, "version");

#define set_integer_const(key, value) do { \
		lua_pushinteger(L, value);         \
		lua_setfield(L, -2, key);          \
	} while(0);

	set_integer_const("AF_PACKET",  AF_PACKET);
	set_integer_const("SOCK_RAW",   SOCK_RAW);
	set_integer_const("SOCK_DGRAM", SOCK_DGRAM);
	
	return 1;
}
