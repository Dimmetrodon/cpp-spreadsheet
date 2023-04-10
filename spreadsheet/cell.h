#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>
#include <stack>
#include <optional>

class Impl;
class EmptyImpl;
class TextImpl;
class FormulaImpl;

class Cell : public CellInterface 
{
public:
    explicit Cell(SheetInterface& sheet);
    ~Cell();

    void                    Set(std::string text);
    void                    Clear();

    Value                   GetValue() const override;
    std::string             GetText() const override;
    std::vector<Position>   GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_;
    std::vector<Cell*> dependent_cells;

    virtual void InvalidateCache();
    void CycleDependentCheck(std::unique_ptr<Impl>& impl) const;

    //Virtual method
class Impl
{
    public:
    virtual                         ~Impl() = default;
    virtual Value                   GetValue() const = 0;
    virtual std::string             GetText() const = 0;
    virtual void                    ClearCache() {};
    virtual std::vector<Position>   GetRefencedCells() const {return {};};
};

class EmptyImpl : public Impl
{
    public:
                                    EmptyImpl() {};
    CellInterface::Value            GetValue() const override;
    std::string                     GetText() const override;
};

class TextImpl : public Impl
{
    public:
                                    TextImpl(std::string text);
    CellInterface::Value            GetValue() const override;
    std::string                     GetText() const override;

    private:
    std::string text_;
};

class FormulaImpl : public Impl
{
    public:

    explicit                        FormulaImpl(const SheetInterface& sheet, std::string formula_text);
    Value                           GetValue() const override;
    std::string                     GetText() const override;
    void                            ClearCache() override;
    std::vector<Position>           GetRefencedCells() const override;

    private:
    const SheetInterface&               sheet_;
    std::unique_ptr<FormulaInterface>   formula_;
    //Cached value. 
    //Whenever GetValue() is used value_ will be used:
    //if value_ empty       - value_ gets filled
    //if value_ not empty   - return value_
    mutable std::optional<FormulaInterface::Value> cache_; 
};
};

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value);