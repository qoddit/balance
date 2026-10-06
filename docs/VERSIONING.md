# Versioning

The product has two related version identifiers:

- `VERSION`: product-facing release identifier, for example `beta-0.3.1`.
- CMake `PROJECT_VERSION`: SemVer core, for example `0.3.1`.

The matching Git tag for `beta-0.3.1` is:

```text
v0.3.1-beta
```

A release should be built only from a clean tree whose `VERSION` file matches the tag. `scripts/release.sh` enforces this.
