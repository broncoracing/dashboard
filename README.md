# template
Use this repo as the base for new repos.
Follow directory structure below:

```bash
├── firmware
│   └── firmware project
└── pcb
    ├── PCB layout
    ├── schematic
    └── etc
```

Upon pushing a commit, a Github action will automatically run electrical & design rule checks to verify the pcb design, then generate gerber/drill/BOM files and renderings/PDFs of the pcb and schematics.
