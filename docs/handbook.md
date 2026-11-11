# Forge2D Engine handbook

By Jason Stys.

Current native work: [Forge2D Editor usage](native-editor.md) and
[actual native validation](reports/native-editor-2026-11-10.md) describe the new 1.1.0 Windows
scene/edit/play/replay workflow. The Word edition below retains its explicitly cited historical
baseline until the separately reviewed updated guide is produced.

The implementation adds bidirectional collision category and mask filters, versioned scene migration, deterministic state hashes and initial/final overlap CSV inspection.

Supports practical game-world collision layers and reproducible debugging against independent geometry oracles.

## Read the documentation

| Edition | Audience and contents |
| --- | --- |
| [project-guide.docx](reports/project-guide.docx) | User and developer guide: purpose, current usage, design, boundaries and source references. |

The editable Markdown remains the maintenance source. The Word files are readable editions, and the workbook is an evidence index rather than a test runner. See [publication identities and review](reports/publications.json) and the [documentation refresh record](reports/documentation-refresh-2026-11-09.md).

## Current usage and help

[README quick start](../README.md) is the entry point. Use the project-specific guides below before changing inputs, operating modes or deployment assumptions.

### Usage and operations

- [operations](operations.md)

### Design and contracts

- [adr/0001 headless core and optional sdl3](adr/0001-headless-core-and-optional-sdl3.md)
- [api](api.md)
- [architecture](architecture.md)
- [complexity](complexity.md)
- [research](research.md)

### Security and validation

- [testing](testing.md)

## Historical results and reproduction

The reviewed source is [`43fd93f79043231230a4c19346bdb8a4c356d9ce`](https://github.com/JasonStys/forge2d-engine/tree/43fd93f79043231230a4c19346bdb8a4c356d9ce). The observed functional workflow baseline is `43fd93f79043231230a4c19346bdb8a4c356d9ce`; those observations are recorded at `2026-11-10T05:25:40.628667+00:00`. They do not certify an unrun later revision.

- [Detailed recorded validation](reports/practical-workflows-20261109.md)
- [Current Actions](https://github.com/JasonStys/forge2d-engine/actions) records later integration checks separately.
- Preserve dated measurements and follow the existing testing guide to reproduce them; a test-source inventory alone does not establish execution.

## Practical boundaries

- Category/mask filtering selects AABB query candidates; it does not add physical collision response, enter/exit callbacks or navigation changes.
- Filter words change numeric state fingerprints even for legacy default filters; replay input grammar remains version 1.
- Raw queries cannot establish current entity-generation validity; World owns lifetime checks. This is a fictional integer 2D game engine.

## Maintaining these editions

Update the editable usage/design/test documents first. Rebuild and visually review every Word page and workbook tab before replacing an edition; verify the exact binary digests and evidence source in publications.json. Keep account data, logs with secrets, installed-app state and private test stores out of documentation. Source and release assets retain their existing security and licensing boundaries.
