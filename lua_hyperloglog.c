/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua hyperloglog implementation @file */

#include <math.h>
#include <string.h>

#include "lauxlib.h"
#include "lua.h"
#include "redis_hyperloglog.h"

#ifdef LUA_SANDBOX
#include "luasandbox_output.h"
#include "luasandbox_serialize.h"
#endif

/* The cached cardinality MSB is used to signal validity of the cached value. */
#define HLL_INVALIDATE_CACHE(hll) (hll)->card[7] |= (1<<7)
#define HLL_VALID_CACHE(hll) (((hll)->card[7] & (1<<7)) == 0)

static const char* mozsvc_hyperloglog = "mozsvc.hyperloglog";

static const char* hll_magic = "HYLL";

static int hyperloglog_new(lua_State* lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 0, 0, "incorrect number of arguments");

  size_t nbytes = sizeof(hyperloglog);
  hyperloglog* hll = (hyperloglog*)lua_newuserdata(lua, nbytes);
  memcpy(hll->magic, hll_magic, sizeof(hll->magic));
  hll->encoding = HLL_DENSE;
  HLL_INVALIDATE_CACHE(hll);
  memset(hll->notused, 0, sizeof(hll->notused));
  memset(hll->registers, 0, HLL_REGISTERS_SIZE);

  luaL_getmetatable(lua, mozsvc_hyperloglog);
  lua_setmetatable(lua, -2);

  return 1;
}


static hyperloglog* check_hyperloglog(lua_State* lua, int args)
{
  void* ud = luaL_checkudata(lua, 1, mozsvc_hyperloglog);
  luaL_argcheck(lua, ud != NULL, 1, "invalid userdata type");
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return (hyperloglog*)ud;
}


static int hyperloglog_add(lua_State* lua)
{
  hyperloglog* hll = check_hyperloglog(lua, 2);
  size_t len = 0;
  double val = 0;
  void* key = NULL;
  switch (lua_type(lua, 2)) {
  case LUA_TSTRING:
    key = (void*)lua_tolstring(lua, 2, &len);
    break;
  case LUA_TNUMBER:
    val = lua_tonumber(lua, 2);
    len = sizeof(double);
    key = &val;
    break;
  default:
    luaL_argerror(lua, 2, "must be a string or number");
    break;
  }

  int altered = 0;
  if (1 == hllDenseAdd(hll->registers, (unsigned char*)key, len)) {
    HLL_INVALIDATE_CACHE(hll);
    altered = 1;
  }

  lua_pushboolean(lua, altered);
  return 1;
}


static int hyperloglog_count(lua_State* lua)
{
  hyperloglog* hll = check_hyperloglog(lua, 1);
  uint64_t card;
  /* Check if the cached cardinality is valid. */
  if (HLL_VALID_CACHE(hll)) {
    /* Just return the cached value. */
    card =  (uint64_t)hll->card[0];
    card |= (uint64_t)hll->card[1] << 8;
    card |= (uint64_t)hll->card[2] << 16;
    card |= (uint64_t)hll->card[3] << 24;
    card |= (uint64_t)hll->card[4] << 32;
    card |= (uint64_t)hll->card[5] << 40;
    card |= (uint64_t)hll->card[6] << 48;
    card |= (uint64_t)hll->card[7] << 56;
  } else {
    /* Recompute it and update the cached value. */
    card = hllCount(hll);
    hll->card[0] = card & 0xff;
    hll->card[1] = (card >> 8) & 0xff;
    hll->card[2] = (card >> 16) & 0xff;
    hll->card[3] = (card >> 24) & 0xff;
    hll->card[4] = (card >> 32) & 0xff;
    hll->card[5] = (card >> 40) & 0xff;
    hll->card[6] = (card >> 48) & 0xff;
    hll->card[7] = (card >> 56) & 0xff;
  }

  lua_pushnumber(lua, (double)card);
  return 1;
}


static int hyperloglog_clear(lua_State* lua)
{
  hyperloglog* hll = check_hyperloglog(lua, 1);
  memset(hll->registers, 0, HLL_REGISTERS_SIZE);
  HLL_INVALIDATE_CACHE(hll);
  return 0;
}


static int hyperloglog_fromstring(lua_State* lua)
{
  hyperloglog* hll = check_hyperloglog(lua, 2);
  size_t len = 0;
  const char* values  = luaL_checklstring(lua, 2, &len);
  if (len != sizeof(hyperloglog) - 1) {
    luaL_error(lua, "fromstring() bytes found: %d, expected %d",
               len, sizeof(hyperloglog) - 1);
  }
  if (memcmp(values, hll_magic, sizeof(hll->magic)) != 0) {
    luaL_error(lua, "fromstring() HYLL header not found");
  }
  if (values[5] != HLL_DENSE) {
    luaL_error(lua, "fromstring() invalid encoding");
  }
  memcpy(hll, values, sizeof(hyperloglog) - 1);
  return 0;
}


static int hyperloglog_version(lua_State* lua)
{
  lua_pushstring(lua, DIST_VERSION);
  return 1;
}


#ifdef LUA_SANDBOX
static int serialize_hyperloglog(lua_State *lua) {
  lsb_output_data* output = (lsb_output_data*)lua_touserdata(lua, -1);
  const char *key = (const char*)lua_touserdata(lua, -2);
  hyperloglog* hll = (hyperloglog*)lua_touserdata(lua, -3);
  if (!(output && key && hll)) {
    return 0;
  }
  if (lsb_appendf(output,
              "if %s == nil then %s = hyperloglog.new() end\n", key, key)) {
    return 1;
  }

  if (lsb_appendf(output, "%s:fromstring(\"", key)) {
    return 1;
  }
  if (lsb_serialize_binary(hll, sizeof(hyperloglog) - 1, output)) return 1;
  if (lsb_appends(output, "\")\n", 3)) {
    return 1;
  }
  return 0;
}


static int output_hyperloglog(lua_State* lua)
{
  lsb_output_data* output = (lsb_output_data*)lua_touserdata(lua, -1);
  hyperloglog* hll = (hyperloglog*)lua_touserdata(lua, -2);
  if (!(output && hll)) {
    return 0;
  }
  return lsb_appends(output, (const char*)hll, sizeof(hyperloglog) - 1);
}
#endif


static const struct luaL_reg hyperlogloglib_f[] =
{
  { "new", hyperloglog_new }
  , { "version", hyperloglog_version }
  , { NULL, NULL }
};


static const struct luaL_reg hyperlogloglib_m[] =
{
  { "add", hyperloglog_add }
  , { "count", hyperloglog_count }
  , { "clear", hyperloglog_clear }
  , { "fromstring", hyperloglog_fromstring } // used for data restoration
  , { NULL, NULL }
};


int luaopen_hyperloglog(lua_State* lua)
{
#ifdef LUA_SANDBOX
  lua_newtable(lua);
  lsb_add_serialize_function(lua, serialize_hyperloglog);
  lsb_add_output_function(lua, output_hyperloglog);
  lua_replace(lua, LUA_ENVIRONINDEX);
#endif
  luaL_newmetatable(lua, mozsvc_hyperloglog);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, hyperlogloglib_m);
  luaL_register(lua, "hyperloglog", hyperlogloglib_f);

  return 1;
}
