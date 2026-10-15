-- Pandoc Lua filter used when converting the LaTeX manuals to rst.
-- It works around two problems in pandoc's rst output.

-- Pandoc 3.x writes a figure's label as "name: id" rather than ":name: id",
-- which docutils reads as part of the image path, so the image is lost.
-- Single-image figures are written directly as rst instead.
function Figure(fig)
  if fig.identifier == "" then return nil end
  local images = {}
  fig.content:walk({ Image = function(img) table.insert(images, img) end })
  if #images ~= 1 then return nil end
  local caption = pandoc.write(pandoc.Pandoc(fig.caption.long), "rst")
  caption = caption:gsub("\n+$", ""):gsub("\n", "\n   ")
  local rst = ".. figure:: " .. images[1].src .. "\n" ..
              "   :name: " .. fig.identifier .. "\n"
  if caption ~= "" then rst = rst .. "\n   " .. caption .. "\n" end
  return pandoc.RawBlock("rst", rst .. "\n")
end

-- Fixes table rows for rst. \rule lines inside cells become horizontal
-- rules, which rst does not allow in tables, so they are dropped. In an rst
-- simple table, a row with an empty first cell is treated as a continuation
-- of the previous row, so such cells get an escaped space, which renders as
-- nothing.
local function fix_rows(rows)
  for _, row in ipairs(rows) do
    for _, c in ipairs(row.cells) do
      c.contents = c.contents:filter(function(block)
        return block.t ~= "HorizontalRule"
      end)
    end
    local cell = row.cells[1]
    if cell and #cell.contents == 0 then
      cell.contents = { pandoc.Plain({ pandoc.RawInline("rst", "\\ ") }) }
    end
  end
end

function Table(tbl)
  fix_rows(tbl.head.rows)
  for _, body in ipairs(tbl.bodies) do
    fix_rows(body.head)
    fix_rows(body.body)
  end
  fix_rows(tbl.foot.rows)
  return tbl
end

-- An \item with no body becomes a definition list term with no definition,
-- which rst merges into the following term. An escaped space, which renders
-- as nothing, gives it a definition of its own.
function DefinitionList(dl)
  for _, item in ipairs(dl.content) do
    local definitions = item[2]
    if #definitions == 0 then
      definitions[1] = {}
    end
    for i, def in ipairs(definitions) do
      if #def == 0 then
        definitions[i] = { pandoc.Plain({ pandoc.RawInline("rst", "\\ ") }) }
      end
    end
  end
  return dl
end

-- Pandoc splits typewriter text such as \ttt{< $<$ $>$} at each math symbol,
-- leaving code fragments that can be just a space, which rst cannot express.
-- Rejoin code that is separated only by single-symbol math.
local function is_symbol(el)
  return el.t == "Math" and el.mathtype == "InlineMath" and
         el.text:match("^%s*[^%w%s\\]%s*$") ~= nil
end

function Inlines(inlines)
  local out = pandoc.Inlines({})
  local i = 1
  while i <= #inlines do
    local el = inlines[i]
    if el.t == "Code" then
      local text = el.text
      local j = i + 1
      while j + 1 <= #inlines and is_symbol(inlines[j])
            and inlines[j + 1].t == "Code" do
        text = text .. inlines[j].text:gsub("%s", "") .. inlines[j + 1].text
        j = j + 2
      end
      out:insert(pandoc.Code(text, el.attr))
      i = j
    else
      out:insert(el)
      i = i + 1
    end
  end
  return out
end
