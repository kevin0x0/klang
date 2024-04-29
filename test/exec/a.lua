-- local sum = 0
-- local i = 0
-- local j = 1
-- while i < 1000000000 do
--   sum = sum + i * j
--   j = -j
--   i = i + 1
-- end
-- print(sum)

-- jit.off()

collectgarbage('stop')
local i = 0
while i < 100000000 do
  local a = "H"
  i = i + 1
end
-- for i = 1, 100000000 do
--   local a = "H"
-- end
