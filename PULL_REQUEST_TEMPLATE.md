## Summary
<!-- One paragraph explaining what this PR changes and why. -->

## Type of Change
- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Refactor (no functional change)
- [ ] Documentation update
- [ ] Build / toolchain change

## Related Issues
<!-- Closes #<issue_number> -->

## Testing
<!-- Describe how you tested the change on hardware or in simulation. -->

| Test | Pass |
|---|---|
| Compiles with zero warnings (`-Wall`) | ☐ |
| CLCD displays correctly after change | ☐ |
| UART output correct at 9600 baud | ☐ |
| RTC time reads correctly | ☐ |
| Keypad EDGE detection stable | ☐ |

## Checklist
- [ ] Code follows the style guide in `CONTRIBUTING.md`
- [ ] New/changed functions have Doxygen comments
- [ ] `LOG.md` updated (if a bug was found/fixed)
- [ ] No new `FOSC` definitions outside `main_config.h`
- [ ] No stdlib identifier conflicts introduced
- [ ] Target branch is `develop`
