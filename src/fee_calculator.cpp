#include "fee_calculator.h"

FeeBreakdown FeeCalculator::calculate(int64_t gross_amount_cents, int risk_score) {
    FeeBreakdown breakdown;
    breakdown.gross_amount_cents = gross_amount_cents;

    // Processing fee:
    // Tier 1 (<= $100.00 / 10,000 cents): 2.9% + 30c
    // Tier 2 (> $100.00 / 10,000 cents): 1.5% + 30c
    if (gross_amount_cents <= 10000) {
        breakdown.processing_fee_cents = (gross_amount_cents * 29) / 1000 + 30;
    } else {
        // BUG: Divides by 100 instead of 1000, charging 15% instead of 1.5%!
        breakdown.processing_fee_cents = (gross_amount_cents * 15) / 100 + 30;
    }

    // Risk withholding:
    // Withhold 10% reserve if risk_score >= 75
    if (risk_score >= 75) {
        breakdown.risk_withheld_cents = (gross_amount_cents * 10) / 100;
    } else {
        breakdown.risk_withheld_cents = 0;
    }

    // BUG: Adds risk_withheld_cents instead of subtracting it!
    breakdown.net_payout_cents = breakdown.gross_amount_cents - breakdown.processing_fee_cents + breakdown.risk_withheld_cents;

    return breakdown;
}
