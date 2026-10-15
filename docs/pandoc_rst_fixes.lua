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

-- In an rst simple table, a row with an empty first cell is treated as a
-- continuation of the previous row. Filling such cells with an escaped
-- space, which renders as nothing, keeps the rows separate.
local function fill_first_cells(rows)
  for _, row in ipairs(rows) do
    local cell = row.cells[1]
    if cell and #cell.contents == 0 then
      cell.contents = { pandoc.Plain({ pandoc.RawInline("rst", "\\ ") }) }
    end
  end
end

function Table(tbl)
  fill_first_cells(tbl.head.rows)
  for _, body in ipairs(tbl.bodies) do
    fill_first_cells(body.head)
    fill_first_cells(body.body)
  end
  fill_first_cells(tbl.foot.rows)
  return tbl
end
