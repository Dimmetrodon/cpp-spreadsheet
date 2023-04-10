#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


// Реализуйте следующие методы
Cell::Cell(SheetInterface& sheet)
    :sheet_(sheet)
    ,impl_(std::make_unique<EmptyImpl>())
{
}

Cell::~Cell() 
{
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetRefencedCells();
}

void Cell::Set(std::string text) 
{
    if (text.empty())
    {
        impl_ = std::make_unique<EmptyImpl>();
    }
    
    if (text[0] == '=' && text.size() != 1)
    {
        std::unique_ptr<Impl> temp_impl = std::make_unique<FormulaImpl>(sheet_, text.substr(1, text.size()));
        CycleDependentCheck(temp_impl);
        swap(impl_, temp_impl);
        
        for (const auto& pos : impl_->GetRefencedCells())
        {
            auto* cell = sheet_.GetCell(pos);
            if (!cell)
            {
                sheet_.SetCell(pos, {});
                cell = sheet_.GetCell(pos);
            }
            static_cast<Cell*>(cell)->dependent_cells.push_back(this);
        }
        InvalidateCache();
    }
    else
    {
        impl_.reset(new TextImpl(text));
    }
}

void Cell::Clear() 
{
    //impl_ = std::move(std::make_unique<EmptyImpl>()); //memory loss?
    //impl_.reset(std::make_unique<EmptyImpl>());
    impl_.reset(new EmptyImpl);
}

Cell::Value Cell::GetValue() const 
{
    return impl_->GetValue();
}

std::string Cell::GetText() const 
{
    return impl_->GetText();
}

// ---------- EmptyImpl ----------
Cell::Value Cell::EmptyImpl::GetValue() const 
{
    return 0.0; //???
}

std::string Cell::EmptyImpl::GetText() const 
{
    return std::string{};
}

// ---------- TextImpl ----------
Cell::TextImpl::TextImpl(std::string text)
    :text_(text)
{
}

Cell::Value Cell::TextImpl::GetValue() const 
{
    if (text_[0] == '\'')
    {
        return text_.substr(1, text_.size());
    }
    return text_;
}

std::string Cell::TextImpl::GetText() const 
{
    return text_;
}

// ---------- FormulaImpl ----------
Cell::FormulaImpl::FormulaImpl(const SheetInterface& sheet, std::string formula_text)
    : sheet_(sheet)
    , formula_(ParseFormula(formula_text))
{
}

Cell::Value Cell::FormulaImpl::GetValue() const 
{
    if (!cache_.has_value())
    {
        cache_ = formula_->Evaluate(sheet_);
    }
    if (std::holds_alternative<double>(*cache_))
    {
        return std::get<double>(*cache_);
    }
    else
    {
        return std::get<FormulaError>(*cache_);
    }
}

std::string Cell::FormulaImpl::GetText() const 
{
    return '=' + formula_->GetExpression();
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value)
{
    if (std::holds_alternative<std::string>(value)) 
    {
        output << std::get<std::string>(value);
    }
    else if (std::holds_alternative<double>(value)) 
    {
        output << std::get<double>(value);
    }
    else 
    {
        output << std::get<FormulaError>(value);
    }
    return output;
}

void Cell::InvalidateCache()
{
    impl_->ClearCache();

    for (Cell* cell_ptr : dependent_cells)
    {
        cell_ptr->InvalidateCache();
    }
}

std::vector<Position> Cell::FormulaImpl::GetRefencedCells() const
{
    return formula_->GetReferencedCells();
}

void Cell::CycleDependentCheck(std::unique_ptr<Impl>& impl) const
{
    std::unordered_set<const Cell*> referenced_cells;
    std::unordered_set<const Cell*> checked_cells;
    std::stack<const Cell*> cells_stack;
    for (const Position& position : impl->GetRefencedCells())
    {
        referenced_cells.insert(reinterpret_cast<Cell*>(sheet_.GetCell(position))); //wtf
    }
    cells_stack.push(this);
    while (!cells_stack.empty())
    {
        const Cell* curr_cell = cells_stack.top();
        if (referenced_cells.count(curr_cell))
        {
            throw CircularDependencyException("Cycle link error");
        }
        cells_stack.pop();
        checked_cells.insert(curr_cell);

        for (const auto* cell : curr_cell->dependent_cells)
        {
            if (!checked_cells.count(cell))
            {
                cells_stack.push(cell);
            }
        }
    }
}

void Cell::FormulaImpl::ClearCache()
{
    cache_.reset();
}