collectgarbage('incremental')

local t = os.clock()

--collectgarbage('stop')
local b = {}
for i = 1, 90000 do
  local key = "key" .. i
  b[key] = key
end
b = nil
for _ = 1, 100000000 do
  local a = { name = 'name' }
end

print(os.clock() - t)
print("memory used: ", collectgarbage('count'))
t = os.clock()
collectgarbage('collect');
print(os.clock() - t)
