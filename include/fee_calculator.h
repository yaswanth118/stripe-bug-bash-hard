#pragma once
#include "types.h"

class FeeCalculator {
public:
    // Calculates the fee breakdown for a transaction:
    //
    // Processing Fee:
    // - Tier 1: For gross amounts <= 10,000 cents ($100.00), fee is 2.9% + 30 cents:
    //           fee = (gross_amount_cents * 29) / 1000 + 30
    // - Tier 2: For gross amounts > 10,000 cents, fee is 1.5% + 30 cents:
    //           fee = (gross_amount_cents * 15) / 1000 + 30
    //
    // Risk Reserve:
    // - If risk_score >= 75, withhold a 10% reserve from payout:
    //           risk_withheld = (gross_amount_cents * 10) / 100
    // - Otherwise, risk_withheld = 0.
    //
    // Net Payout:
    // - net_payout = gross_amount_cents - processing_fee_cents - risk_withheld_cents
    static FeeBreakdown calculate(int64_t gross_amount_cents, int risk_score);
};
