collectgarbage('incremental')

local t = os.clock()

-- local _G = _G
-- for i = 1, 90000 do
--   local v = "key" .. i
--   _G[v] = v
-- end

for i = 1, 100000000 do
  local a = {}
  a[1] = i;
end

print(os.clock() - t)
print("memory used: ", collectgarbage('count'))
t = os.clock()
collectgarbage('collect');
print(os.clock() - t)
