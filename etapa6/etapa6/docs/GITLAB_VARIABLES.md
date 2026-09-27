# Variabile GitLab CI/CD (Settings → CI/CD → Variables)

| Variabila         | Exemplu                  | Protected | Masked |
|-------------------|--------------------------|-----------|--------|
| DEPLOY_HOST       | 192.168.1.100            | ✅        | ❌     |
| DEPLOY_USER       | ubuntu                   | ✅        | ❌     |
| DEPLOY_SSH_KEY    | -----BEGIN OPENSSH...    | ✅        | ✅     |
| DOMAIN            | sera.domeniultau.ro      | ✅        | ❌     |
| SLACK_WEBHOOK_URL | https://hooks.slack.com/ | ✅        | ✅     |

## Generare SSH key pentru deploy:
```bash
ssh-keygen -t ed25519 -C "gitlab-deploy" -f ~/.ssh/gitlab_deploy
# Adauga cheia publica pe server:
ssh-copy-id -i ~/.ssh/gitlab_deploy.pub ubuntu@192.168.1.100
# Adauga cheia privata in GitLab Variable DEPLOY_SSH_KEY
cat ~/.ssh/gitlab_deploy
```
