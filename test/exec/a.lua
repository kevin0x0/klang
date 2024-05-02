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

-- local i = 0
-- while i < 100000000 do
--   local a = "H"
--   i = i + 1
-- end
--
collectgarbage('incremental')

for i = 1, 1000000 do
  local key = "" .. i
  _G[key] = key
end

for i = 1, 100000000 do
  local a = { name = "name" }
end

-- for k, v in pairs(_G) do
--   print(k, v)
-- end

-- local function f(n)
--   return n <= 1 and n or f(n - 1) + f(n - 2)
-- end
-- 
-- print(f(36))
