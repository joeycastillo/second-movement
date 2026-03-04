#!/bin/bash
# Security Fix Script for PR #63
# Restores input validation that was removed between ddff5dd and HEAD

set -e  # Exit on error

REPO_DIR="$HOME/repos/second-movement"
BRANCH="phase3-pr5-builder-ui"
FIX_COMMIT="ddff5dd"

echo "🔒 Security Fix Script for PR #63"
echo "================================="
echo ""

# Check if we're in the right directory
if [ ! -d "$REPO_DIR/.git" ]; then
    echo "❌ Error: Repository not found at $REPO_DIR"
    exit 1
fi

cd "$REPO_DIR"

# Check if we're on the right branch
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$CURRENT_BRANCH" != "$BRANCH" ]; then
    echo "⚠️  Warning: Not on $BRANCH (currently on $CURRENT_BRANCH)"
    read -p "Switch to $BRANCH? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        git checkout "$BRANCH"
    else
        echo "❌ Aborted - please switch to $BRANCH manually"
        exit 1
    fi
fi

echo "✅ On branch: $BRANCH"
echo ""

# Check for uncommitted changes
if ! git diff-index --quiet HEAD --; then
    echo "⚠️  Warning: You have uncommitted changes"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "❌ Aborted - commit or stash your changes first"
        exit 1
    fi
fi

echo "📋 Applying security fix from commit $FIX_COMMIT..."
echo ""

# Option 1: Try cherry-pick first (cleanest)
if git cherry-pick "$FIX_COMMIT" 2>/dev/null; then
    echo "✅ Cherry-pick successful!"
    echo ""
    echo "🔍 Verification:"
    echo ""
    
    # Verify the fix is present
    if grep -q "isNaN(val) || val < 15 || val > 35" builder/index.html; then
        echo "  ✅ Zone emergence validation present"
    else
        echo "  ❌ Zone emergence validation MISSING"
    fi
    
    if grep -q "isNaN(val) || val < 40 || val > 60" builder/index.html; then
        echo "  ✅ Zone momentum validation present"
    else
        echo "  ❌ Zone momentum validation MISSING"
    fi
    
    if grep -q "isNaN(val) || val < 65 || val > 85" builder/index.html; then
        echo "  ✅ Zone active validation present"
    else
        echo "  ❌ Zone active validation MISSING"
    fi
    
    if grep -q "isNaN(score) || score < 0 || score > 100" builder/index.html; then
        echo "  ✅ Preview score validation present"
    else
        echo "  ❌ Preview score validation MISSING"
    fi
    
    if grep -q "encodeURIComponent(state.zoneEmergenceMax)" builder/index.html; then
        echo "  ✅ URL encoding present"
    else
        echo "  ❌ URL encoding MISSING"
    fi
    
    if grep -q "const clampZone = (val, def, min, max)" builder/index.html; then
        echo "  ✅ Re-validation before build present"
    else
        echo "  ❌ Re-validation before build MISSING"
    fi
    
    echo ""
    echo "✅ Security fix applied successfully!"
    echo ""
    echo "📤 Next steps:"
    echo "  1. Review the changes: git show HEAD"
    echo "  2. Run manual tests (see SECURITY_REVIEW_PR63.md)"
    echo "  3. Push to remote: git push origin $BRANCH"
    echo ""
    exit 0
else
    echo "⚠️  Cherry-pick failed (conflicts or already applied)"
    echo ""
    echo "🔄 Attempting alternative method..."
    echo ""
    
    # Option 2: Apply patch manually
    git show "$FIX_COMMIT" -- builder/index.html > /tmp/security-fix-pr63.patch
    
    if git apply --check /tmp/security-fix-pr63.patch 2>/dev/null; then
        git apply /tmp/security-fix-pr63.patch
        git add builder/index.html
        git commit -m "fix: Restore input validation for zone configuration UI (from $FIX_COMMIT)

Security fixes for PR #63:
- Add input validation to zone slider event handlers
- Add validation to zone preview score input
- Add encodeURIComponent to URL hash for zone parameters
- Re-validate all zone values before sending to backend

This restores the security fix that was inadvertently removed."
        
        echo "✅ Patch applied and committed!"
        echo ""
        echo "📤 Next steps:"
        echo "  1. Review the changes: git show HEAD"
        echo "  2. Run manual tests (see SECURITY_REVIEW_PR63.md)"
        echo "  3. Push to remote: git push origin $BRANCH"
        echo ""
    else
        echo "❌ Automatic fix failed"
        echo ""
        echo "Manual fix required:"
        echo "  1. Review the patch: git show $FIX_COMMIT -- builder/index.html"
        echo "  2. Manually edit builder/index.html"
        echo "  3. See SECURITY_REVIEW_PR63.md for specific changes needed"
        echo ""
        exit 1
    fi
fi
