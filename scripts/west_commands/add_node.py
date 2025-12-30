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

            This command will:
            - Detect module.yml to get module name and dependencies
            - Add the module to the nearest west.yml projects list
            - Support both local paths and git repositories

            Examples:
                # Add local module
                west one add-node /path/to/my-motor
                west one add-node modules/lib/my-motor

                # Add git repository
                west one add-node https://github.com/user/my-motor.git
                west one add-node --name my-motor --remote origin --revision main
            '''))

    # 位置参数：模块路径或URL
    parser.add_argument(
        'path',
        help='path to Node module directory or git repository URL'
    )

    # Git相关参数
    parser.add_argument(
        '--name',
        help='module name (default: auto-detect from module.yml or directory name)'
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
        help='target path in workspace (default: modules/lib/<name>)'
    )

    # 可选参数：manifest位置
    parser.add_argument(
        '--manifest',
        help='path to west.yml to modify (default: auto-detect)'
    )

    return parser


def run_add_node(cmd, args):
    """执行add-node命令"""

    # 1. 解析路径/URL
    is_git_url = GitHelper.is_git_url(args.path)

    if is_git_url:
        # Git仓库模式
        module_info = parse_git_info(cmd, args)
    else:
        # 本地路径模式
        module_path = Path(args.path).resolve()
        if not module_path.exists():
            cmd.die(f"Module path does not exist: {module_path}")

        module_info = parse_local_module(cmd, module_path, args)

    # 2. 验证模块
    validate_module(cmd, module_info)

    # 3. 查找west.yml
    if args.manifest:
        manifest_path = Path(args.manifest)
    else:
        try:
            manifest_path = WorkspaceHelper.find_manifest()
        except FileNotFoundError:
            cmd.die("Could not find west.yml manifest")

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


def parse_local_module(cmd, module_path: Path, args) -> dict:
    """解析本地模块信息"""
    # 读取module.yml
    module_yml_path = module_path / 'zephyr' / 'module.yml'
    if not module_yml_path.exists():
        cmd.die(f"Not a valid Zephyr module: missing {module_yml_path}")

    module_yml = YamlHandler.load(module_yml_path)

    # 提取模块名称
    module_name = args.name or module_yml.get('name') or module_path.name

    # 检查是否是git仓库
    is_git = (module_path / '.git').exists()

    if is_git:
        # 获取git信息
        git_info = GitHelper.get_remote_info(module_path)

        return {
            'name': module_name,
            'path': args.target_path or f'modules/lib/{module_name}',
            'remote': args.remote or git_info.get('remote', 'origin'),
            'repo-path': args.repo_path or git_info.get('repo_path'),
            'revision': args.revision,
            'url': git_info.get('url'),
            'dependencies': module_yml.get('dependencies', []),
        }
    else:
        # 本地模块（非git）- 使用相对路径
        workspace_root = WorkspaceHelper.find_root(cmd.manifest)
        try:
            rel_path = module_path.relative_to(workspace_root)
        except ValueError:
            rel_path = module_path

        return {
            'name': module_name,
            'path': str(rel_path),
            'is_local': True,
            'dependencies': module_yml.get('dependencies', []),
        }


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
        'path': args.target_path or f'modules/lib/{args.name or default_name}',
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
    if module_info.get('is_local'):
        # 本地模块：只需要name和path
        project = {
            'name': module_info['name'],
            'path': module_info['path'],
        }
    else:
        # Git模块
        project = {
            'name': module_info['name'],
            'path': module_info['path'],
        }

        if module_info.get('remote'):
            project['remote'] = module_info['remote']

        if module_info.get('revision'):
            project['revision'] = module_info['revision']

        if module_info.get('repo-path'):
            project['repo-path'] = module_info['repo-path']

    # 添加到projects列表
    manifest_data['manifest']['projects'].append(project)

    cmd.inf(f"Added project: {module_info['name']}")
