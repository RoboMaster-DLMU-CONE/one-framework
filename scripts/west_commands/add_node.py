"""
west one add-node command implementation
"""

import argparse
import textwrap
import sys
import os
from pathlib import Path

# Add current directory to path to support both relative and absolute imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from node_utils import GitHelper, WorkspaceHelper, YamlHandler


def add_add_node_parser(subparsers, parent_command):
    """添加add-node子命令的参数解析器"""
    parser = subparsers.add_parser(
        'add-node',
        help='add an existing Node module to west.yml manifest',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent('''\
            Add an existing Node module to the west manifest (west.yml).

            This command only supports adding modules from remote git repositories.
            The repository will be added to the manifest projects list with a
            default path of modules/lib/nodes/{name} unless --target-path is given.

            Examples:
                # Add git repository
                west one add-node https://github.com/user/my-motor.git
                west one add-node --name my-motor --remote origin --revision main
            '''))

    # 位置参数：模块git URL
    parser.add_argument(
        'path',
        help='git repository URL for the Node module'
    )

    # Git相关参数
    parser.add_argument(
        '--name',
        help='module name (default: auto-detect from repository URL)'
    )

    parser.add_argument(
        '--remote',
        help='remote name for git repository (default: infer from URL or "origin")'
    )

    parser.add_argument(
        '--revision',
        default='main',
        help='git revision/branch (default: main)'
    )

    parser.add_argument(
        '--repo-path',
        help='repository path (for GitHub: org/repo format)'
    )

    parser.add_argument(
        '--target-path',
        help='target path in workspace (default: modules/lib/nodes/<name>)'
    )

    # 可选参数：manifest位置
    parser.add_argument(
        '--manifest',
        help='path to west.yml to modify (default: auto-detect)'
    )

    return parser


def run_add_node(cmd, args):
    """执行add-node命令 -- only accept git URLs"""

    # 1. 解析路径/URL - 仅支持git URL
    is_git_url = GitHelper.is_git_url(args.path)

    if not is_git_url:
        cmd.die("Only git repository URLs are supported. Provide a git URL (e.g. https://github.com/org/repo.git)")

    module_info = parse_git_info(cmd, args)

    # 2. 验证模块
    validate_module(cmd, module_info)

    # 3. 查找west.yml
    manifest_path = None
    if args.manifest:
        manifest_path = Path(args.manifest)
    else:
        try:
            manifest_path = WorkspaceHelper.find_manifest()
        except FileNotFoundError:
            cmd.die("Could not find west.yml manifest")
            return

    if not manifest_path.exists():
        cmd.die(f"Manifest file does not exist: {manifest_path}")

    # 4. 读取并解析west.yml
    manifest_data = YamlHandler.load(manifest_path)

    # 5. 检查是否已存在
    if module_exists_in_manifest(manifest_data, module_info['name']):
        cmd.wrn(f"Module '{module_info['name']}' already exists in manifest")
        response = input("Overwrite? [y/N]: ").strip().lower()
        if response != 'y':
            cmd.inf("Cancelled")
            return
        # 移除旧条目
        remove_module_from_manifest(manifest_data, module_info['name'])

    # 6. 添加到manifest
    add_module_to_manifest(cmd, manifest_data, module_info)

    # 7. 保存west.yml
    YamlHandler.save(manifest_path, manifest_data)

    # 8. 显示成功信息
    cmd.banner(f"Successfully added module: {module_info['name']}")
    cmd.inf(f"Manifest: {manifest_path}")
    cmd.inf("\nNext steps:")
    cmd.inf("  1. Run: west update")
    if module_info.get('config_name'):
        cmd.inf(f"  2. Enable in Kconfig: CONFIG_{module_info['config_name']}=y")


def parse_git_info(cmd, args) -> dict:
    """解析Git仓库信息"""
    url = args.path

    # 解析URL获取remote和repo-path
    github_info = GitHelper.parse_github_url(url)

    if github_info:
        org, repo = github_info
        default_name = repo
        default_repo_path = f'{org}/{repo}'
    else:
        default_name = url.split('/')[-1].replace('.git', '')
        default_repo_path = None

    return {
        'name': args.name or default_name,
        # 默认 path 更改为 modules/lib/nodes/{name}
        'path': args.target_path or f'modules/lib/nodes/{args.name or default_name}',
        'remote': args.remote or 'origin',
        'repo-path': args.repo_path or default_repo_path,
        'revision': args.revision,
        'url': url,
    }


def validate_module(cmd, module_info: dict):
    """验证模块信息"""
    if not module_info.get('name'):
        cmd.die("Module name could not be determined")


def module_exists_in_manifest(manifest_data: dict, module_name: str) -> bool:
    """检查模块是否已存在于manifest中"""
    if 'manifest' not in manifest_data:
        return False
    if 'projects' not in manifest_data['manifest']:
        return False

    for project in manifest_data['manifest']['projects']:
        if project.get('name') == module_name:
            return True

    return False


def remove_module_from_manifest(manifest_data: dict, module_name: str):
    """从manifest中移除模块"""
    if 'manifest' not in manifest_data:
        return
    if 'projects' not in manifest_data['manifest']:
        return

    manifest_data['manifest']['projects'] = [
        p for p in manifest_data['manifest']['projects']
        if p.get('name') != module_name
    ]


def add_module_to_manifest(cmd, manifest_data: dict, module_info: dict):
    """将模块添加到manifest的projects列表"""
    if 'manifest' not in manifest_data:
        manifest_data['manifest'] = {}

    if 'projects' not in manifest_data['manifest']:
        manifest_data['manifest']['projects'] = []

    # 构建project条目
    project = {
        'name': module_info['name'],
        'path': module_info['path'],
    }

    # 如果有git URL，添加url字段
    if module_info.get('url'):
        project['url'] = module_info['url']

    # 添加revision
    if module_info.get('revision'):
        project['revision'] = module_info['revision']

    # 添加到projects列表
    manifest_data['manifest']['projects'].append(project)

    cmd.inf(f"Added project: {module_info['name']}")
