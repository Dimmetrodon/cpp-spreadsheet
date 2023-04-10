#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) 
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Wrong position");
    }
    if (pos.row + 1 > static_cast<int>(sheet_.size()))
    {
        sheet_.resize(pos.row + 1);
    }
    if (pos.col + 1 > static_cast<int>(sheet_[pos.row].size()))
    {
        sheet_[pos.row].resize(pos.col + 1);
    }
    if (!sheet_[pos.row][pos.col])
    {
        sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this);
        sheet_[pos.row][pos.col]->Set(std::move(text));
    }
    else
    {
        sheet_[pos.row][pos.col]->Set(std::move(text));
    }
}

const CellInterface* Sheet::GetCell(Position pos) const 
{
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) 
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Wrong position");
    }
    if ((pos.row >= static_cast<int>(sheet_.size())) || (pos.col >= static_cast<int>(sheet_[pos.row].size())))
    {
        return nullptr;
    }
    return sheet_[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos) 
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Wrong position");
    }
    const Cell* cell_ptr = reinterpret_cast<const Cell*>(GetCell(pos));
    if (cell_ptr)
    {
        (*sheet_[pos.row][pos.col]).Clear();
        sheet_[pos.row][pos.col].reset();
    }
}

Size Sheet::GetPrintableSize() const 
{
    Size result {0, 0};
    for (int row = 0; row != static_cast<int>(sheet_.size()); ++row)
    {
        for (int col = sheet_[row].size() - 1; col >= 0; --col)
        {
            const Cell* cell_ptr = reinterpret_cast<const Cell*>(GetCell({row, col}));
            if (cell_ptr)
            {
                if (!sheet_[row][col]->GetText().empty())
                {
                    if (row + 1 > result.rows)
                    {
                        result.rows = row + 1;
                    }
                    if (col + 1 > result.cols)
                    {
                        result.cols = col + 1;
                    }
                    break;
                }
            }
        }
    }
    return result;
}

void Sheet::PrintValues(std::ostream& output) const 
{
    Size size = GetPrintableSize();

    for (int row = 0; row != size.rows; ++row)
    {
        for (int col = 0; col != size.cols; ++col)
        {
            const Cell* cell_ptr = reinterpret_cast<const Cell*>(GetCell({row, col}));
            if (cell_ptr)
            {
                output << cell_ptr->GetValue();
            }
            if (col + 1 != size.cols) 
            {
                output << "\t";
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const 
{
    Size size = GetPrintableSize();

    for (int row = 0; row != size.rows; ++row)
    {
        for (int col = 0; col != size.cols; ++col)
        {
            if (col > 0)
            {
                output << '\t';
            }
            const Cell* cell_ptr = reinterpret_cast<const Cell*>(GetCell({row, col}));
            if (cell_ptr)
            {
                output << cell_ptr->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() 
{
    return std::make_unique<Sheet>();
}