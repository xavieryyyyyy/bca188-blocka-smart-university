# Contributing to SENTRA

> **[Team & AI Guide](AGENTS.md)**
>
> Find your station’s responsibilities, the checks to finish before integration, and a prompt you can paste into ChatGPT.

Work from this Block A repository and use your own GitHub account. The team lead grants write access. Each student should own or co-own a node or dashboard feature and make commits that describe what they changed.

## Make your first contribution

Clone and open the repository.

```sh
git clone https://github.com/xavieryyyyyy/bca188-blocka-smart-university.git
cd bca188-blocka-smart-university
git switch -c andam/classroom-1-setup
```

This example branch is for Classroom 1. Choose a name that matches your own task, such as `hayag/window-light` or `dashboard/node-status`. Use the subsystem folders below to find your starting point.

1. Edit your assigned files. If your work adds a reading, agree on the registry entry before using the class broker.
2. Check what you changed with `git status` and `git diff`.
3. Stage the files you changed with `git add <path>`. Review them with `git diff --staged`.
4. Commit with a plain description, such as `git commit -m "Set up Classroom 1 help button"`.
5. Publish your branch with `git push -u origin HEAD`, then open a pull request to `main`.

In the pull request, describe the change, which board or screen it affects, and what you tested. Attach a screenshot or a short test result when it helps a reviewer. Test results should say whether you checked the physical device, the class broker, or the dashboard.

## Find your files

- [ESP32 stations](nodes/)
- [Campus backend](backend/)
- [Dashboard](dashboard/)
- [Data registry](DATA_REGISTRY.md)
- [Security checks](SECURITY.md)
- [Test results](TESTING.md)
- [Presentation](slides/)

Use `secrets.h` and `.env` for local credentials. Use sample files with placeholders when teammates need setup instructions.
