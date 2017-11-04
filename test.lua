require "hyperloglog"
require "string"

local hll = hyperloglog.new()
local expected = 0
assert(hll:count() == expected, string.format("incorect count expected: %d, received: %d", expected, hll:count()))

hll:add("one")
hll:add("two")
expected = 2
assert(hll:count() == expected, string.format("incorect count expected: %d, received: %d", expected, hll:count()))

hll:clear()

expected = 0
assert(hll:count() == expected, string.format("incorect count expected: %d, received: %d", expected, hll:count()))

local ok, _ = pcall(hyperloglog.new, 2)
assert(ok == false, "new incorrect number of args")

ok, _ = pcall(hll.add, hll, {})
assert(ok == false, "add incorrect argument type")
ok, _ = pcall(hll.fromstring, hll, {})
assert(ok == false, "fromstring incorrect argument type")
ok, _ = pcall(hll.fromstring, hll, "      ")
assert(ok == false, "fromstring incorrect argument length")
ok, _ = pcall(hll.add, hll)
assert(ok == false, "add incorrect number of args")
ok, _ = pcall(hll.count, hll, 1)
assert(ok == false, "count incorrect number of args")
ok, _ = pcall(hll.clear, hll, 1)
assert(ok == false, "clear incorrect number of args")
