# Triage Labels

The skills speak in terms of five canonical triage roles. This file maps those roles to Linear issue labels for project `Unreal_myika` under the `Myika AI` team.

| Label in mattpocock/skills | Label in our backlog | Meaning                                  |
| -------------------------- | -------------------- | ---------------------------------------- |
| `needs-triage`             | `needs-triage`       | Maintainer needs to evaluate this issue  |
| `needs-info`               | `needs-info`         | Waiting on reporter for more information |
| `ready-for-agent`          | `ready-for-agent`    | Fully specified, ready for an AFK agent  |
| `ready-for-human`          | `ready-for-human`    | Requires human implementation            |
| `wontfix`                  | `wontfix`            | Will not be actioned                     |

When a skill mentions a triage role, use the mapped string above on the Linear issue.

For local queue tasks:

- record the practical state in the queue task frontmatter/body
- keep the canonical role wording in notes or related Linear metadata when that helps future routing
- do not invent a second repo-local label system just for this project
