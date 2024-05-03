collectgarbage("incremental")
collectgarbage("stop")
local time = os.clock()
local t = {}
for i = 1, 10000000 do
  t["" .. i] = i
end
print(os.clock() - time)
