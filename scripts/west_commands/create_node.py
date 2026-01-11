"""
west one create-node command implementation
"""

import argparse
import shutil
import textwrap
import sys
import os
from pathlib import Path
from urllib.parse import urlparse
import subprocess

# Add current directory to path to support both relative and absolute imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from node_utils import NameConverter, WorkspaceHelper, TemplateEngine


def add_create_node_parser(subparsers, parent_command):
    """添加create-node子命令的参数解析器

    改动：移除 `name` 位置参数，改为将 git url 作为位置参数；移除可选 `--git-url`。
    """
    parser = subparsers.add_parser(
        'create-node',
        help='create a new Node module from a git repository URL',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent('''\
            Create a new OneFramework Node module by specifying a git repository URL.

            The repository name will be used as the module name.

            Example:
                west one create-node https://github.com/your-org/my-node.git
                west one create-node https://github.com/your-org/my-node --output custom/path
            '''))

    # 位置参数：git 仓库 URL（必填）
    parser.add_argument(
        'git_url',
        help='git repository URL (e.g., https://github.com/org/repo.git)'
    )

    # 可选参数：输出路径
    parser.add_argument(
        '-o', '--output',
        help='output directory (default: modules/lib/nodes/<name>)',
        default=None
    )

    # 可选参数：Topic数据类型名
    parser.add_argument(
        '--data-type',
        help='Topic data type name (default: derived from module name)',
        default=None
    )

    # 可选参数：Meta配置
    parser.add_argument(
        '--stack-size',
        type=int,
        default=2048,
        help='thread stack size in bytes (default: 2048)'
    )

    parser.add_argument(
        '--priority',
        type=int,
        default=5,
        help='thread priority (default: 5)'
    )

    parser.add_argument(
        '--meta-name',
        help='Meta::name value (default: derived from module name)',
        default=None
    )

    # 可选参数：Topic配置
    parser.add_argument(
        '--topic-name',
        help='Topic variable name (default: topic_<snake_case>)',
        default=None
    )

    parser.add_argument(
        '--topic-string',
        help='Topic identifier string (default: <snake_case>_data)',
        default=None
    )

    # 可选参数：强制覆盖
    parser.add_argument(
        '-f', '--force',
        action='store_true',
        help='force overwrite if directory exists'
    )

    # 可选参数：添加到west.yml
    parser.add_argument(
        '--add-to-manifest',
        action='store_true',
        help='automatically add module to nearest west.yml manifest'
    )

    # Git/Remote相关参数
    parser.add_argument(
        '--init-git',
        action='store_true',
        help='initialize git repository in module directory for version control'
    )

    return parser


def extract_repo_name_from_url(git_url: str) -> str:
    """从 git url 中提取仓库名（不带 .git 后缀）"""
    try:
        parsed = urlparse(git_url)
        path = parsed.path
        name = Path(path).name
    except Exception:
        # 兜底解析
        name = git_url.rstrip('/').rsplit('/', 1)[-1]

    if name.endswith('.git'):
        name = name[:-4]
    return name


def run_create_node(cmd, args):
    """执行create-node命令"""

    # 1. git url 必填，解析并从中生成 module name
    git_url = args.git_url
    if not git_url:
        cmd.die('git repository URL is required as the positional argument')

    repo_name = extract_repo_name_from_url(git_url)
    module_name_kebab = NameConverter.to_kebab_case(repo_name)

    # 验证并规范化名称
    valid, error = NameConverter.validate_name(module_name_kebab)
    if not valid:
        cmd.die(f"Invalid module name derived from URL '{repo_name}': {error}")

    module_name_pascal = NameConverter.to_pascal_case(module_name_kebab)
    module_name_snake = NameConverter.to_snake_case(module_name_kebab)
    module_name_upper = NameConverter.to_upper_snake_case(module_name_kebab)

    # 2. 确定输出路径（总是默认 modules/lib/nodes/<name>，除非用户通过 -o/--output 覆盖）
    if args.output:
        output_dir = Path(args.output).resolve()
    else:
        # 查找workspace根目录
        workspace_root = WorkspaceHelper.find_root(cmd.manifest)
        output_dir = workspace_root / 'modules' / 'lib' / 'nodes' / module_name_kebab

    # 3. 检查目标目录是否存在
    if output_dir.exists():
        if not args.force:
            cmd.die(f"Directory already exists: {output_dir}\nUse --force to overwrite")
        else:
            cmd.wrn(f"Overwriting existing directory: {output_dir}")
            shutil.rmtree(output_dir)

    # 4. 创建目录结构
    cmd.inf(f"Creating Node module: {module_name_kebab}")
    create_directory_structure(cmd, output_dir)

    # 5. 生成配置参数（基于 git_url）
    config = build_template_config(args, {
        'module_name_kebab': module_name_kebab,
        'module_name_pascal': module_name_pascal,
        'module_name_snake': module_name_snake,
        'module_name_upper': module_name_upper,
        'git_url': git_url,
    })

    # 6. 生成所有文件
    generate_files(cmd, output_dir, config)

    # 7. 初始化git仓库（如果请求或者提供了 git_url）
    git_remote_added = False
    # 如果用户要求初始化本地仓库，则执行 git init，并配置 origin 为传入的 git_url
    if args.init_git:
        git_remote_added = init_git_repo(cmd, output_dir, git_url)

    # 8. 可选：添加到manifest（使用 git url）
    if args.add_to_manifest:
        add_to_west_manifest(cmd, output_dir, module_name_kebab, git_url)

    # 9. 显示成功信息
    cmd.banner(f"Successfully created Node module: {module_name_kebab}")
    cmd.inf(f"Location: {output_dir}")
    cmd.inf("\nNext steps:")
    cmd.inf(f"  1. Implement your node logic:")
    cmd.inf(f"     - src/{module_name_pascal}Node.cpp")
    cmd.inf(f"     - include/{config['DataClass']}.hpp")

    cmd.inf(f"\n  2. Make further changes and push to remote:")
    cmd.inf(f"     cd {output_dir}")
    cmd.inf(f"     git add .")
    cmd.inf(f"     git commit -m '<your commit message>'")
    cmd.inf(f"     git push origin main")
    cmd.inf(f"\n  3. Add module to workspace manifest:")
    if not args.add_to_manifest:
        cmd.inf(f"     west one add-node {output_dir}")
    cmd.inf(f"\n  4. Enable in Kconfig: CONFIG_{module_name_upper}=y")

    cmd.inf(f"\nGit information:")
    cmd.inf(f"  - Repository: {git_url}")
    if git_remote_added:
        cmd.inf(f"  - Remote 'origin' configured ✓")
        cmd.inf(f"  - Initial commit pushed ✓")
        cmd.inf(f"  - Module west.yml created with git configuration ✓")


def create_directory_structure(cmd, base_dir: Path):
    """创建标准目录结构"""
    dirs = [
        base_dir,
        base_dir / 'src',
        base_dir / 'include',
        base_dir / 'zephyr',
    ]

    for dir_path in dirs:
        dir_path.mkdir(parents=True, exist_ok=True)
        cmd.dbg(f"Created directory: {dir_path}")


def build_template_config(args, names: dict) -> dict:
    """构建模板替换配置

    现在假定总是有 git_url，并从中解析 remote 基础 URL 和仓库信息。
    """
    # 提取基础名称
    module_name_kebab = names['module_name_kebab']
    module_name_pascal = names['module_name_pascal']
    module_name_snake = names['module_name_snake']
    module_name_upper = names['module_name_upper']
    git_url = names.get('git_url', '')

    # 从 git_url 中解析 base url（例如 https://github.com/org/repo.git -> https://github.com/org）
    remote_base_url = ''
    try:
        parts = git_url.rstrip('/').rsplit('/', 1)
        remote_base_url = parts[0] if len(parts) > 1 else git_url
    except Exception:
        remote_base_url = git_url

    # 生成 self project 条目（用于模板），包含 path 到 modules/lib/nodes/<name>
    self_project = f"\n    - name: {module_name_kebab}\n      url: {git_url}\n      path: modules/lib/nodes/{module_name_kebab}\n      revision: main"

    # 构建配置字典
    config = {
        'MODULE_NAME': module_name_kebab,
        'CONFIG_NAME': module_name_upper,
        'NodeClass': module_name_pascal + 'Node',
        'DataClass': args.data_type or (module_name_pascal + 'Data'),
        'DESCRIPTION': f"{module_name_kebab} node for OneFramework",

        # Meta配置
        'STACK_SIZE': args.stack_size,
        'PRIORITY': args.priority,
        'META_NAME': args.meta_name or module_name_snake,

        # Topic配置
        'TOPIC_VAR': args.topic_name or f'topic_{module_name_snake}',
        'TOPIC_STRING': args.topic_string or f'{module_name_snake}_data',

        # 头文件保护宏
        'HEADER_GUARD': (args.data_type or (module_name_pascal + 'Data')).upper() + '_HPP',

        # Git配置（用于模板）
        'REMOTE_BASE_URL': remote_base_url,
        'SELF_PROJECT': self_project,
        'GIT_URL': git_url,
    }

    return config


def generate_files(cmd, output_dir: Path, config: dict):
    """从模板生成所有文件"""
    template_dir = Path(__file__).parent / 'templates'

    # 文件映射：(模板文件名, 输出文件路径)
    file_mappings = [
        ('CMakeLists.txt.template', output_dir / 'CMakeLists.txt'),
        ('Kconfig.template', output_dir / 'zephyr' / 'Kconfig'),
        ('module.yml.template', output_dir / 'zephyr' / 'module.yml'),
        ('node.cpp.template', output_dir / 'src' / f"{config['NodeClass']}.cpp"),
        ('data.hpp.template', output_dir / 'include' / f"{config['DataClass']}.hpp"),
        ('gitignore.template', output_dir / '.gitignore'),
        ('west.yml.template', output_dir / 'west.yml'),
    ]

    for template_name, output_path in file_mappings:
        template_path = template_dir / template_name
        if not template_path.exists():
            cmd.wrn(f"Template not found: {template_path}")
            continue

        TemplateEngine.render_file(template_path, output_path, config)
        cmd.dbg(f"Generated: {output_path}")


def add_to_west_manifest(cmd, module_path: Path, module_name: str, git_url: str):
    """将模块添加到west.yml manifest（使用 git url，而不是仅本地子模块路径）"""
    try:
        from node_utils import WorkspaceHelper, YamlHandler

        # 查找west.yml
        try:
            manifest_path = WorkspaceHelper.find_manifest()
        except FileNotFoundError:
            cmd.wrn("Could not find west.yml manifest, skipping auto-add")
            return

        # 读取manifest
        manifest_data = YamlHandler.load(manifest_path)

        # 确保manifest结构存在
        if 'manifest' not in manifest_data:
            manifest_data['manifest'] = {}
        if 'projects' not in manifest_data['manifest']:
            manifest_data['manifest']['projects'] = []

        # 计算相对路径（默认放到 modules/lib/nodes/<name>）
        workspace_root = WorkspaceHelper.find_root(cmd.manifest)
        try:
            rel_path = module_path.relative_to(workspace_root)
        except ValueError:
            rel_path = module_path

        # 检查是否已存在
        for project in manifest_data['manifest']['projects']:
            if project.get('name') == module_name:
                cmd.wrn(f"Module '{module_name}' already exists in manifest")
                return

        # 添加新项目（使用 git url）
        project = {
            'name': module_name,
            'url': git_url,
            'path': str(rel_path),
            'revision': 'main',
        }
        manifest_data['manifest']['projects'].append(project)

        # 保存manifest
        YamlHandler.save(manifest_path, manifest_data)
        cmd.inf(f"Added module to manifest: {manifest_path}")

    except Exception as e:
        cmd.wrn(f"Failed to add module to manifest: {e}")


def init_git_repo(cmd, module_path: Path, git_url: str = None) -> bool:
    """初始化本地git仓库并配置remote，返回是否成功添加remote"""
    try:

        # 初始化git仓库
        subprocess.run(
            ['git', 'init'],
            cwd=module_path,
            capture_output=True,
            check=True
        )
        cmd.dbg(f"Initialized git repository in {module_path}")

        remote_added = False

        # 如果提供了git URL，添加为remote
        if git_url:
            try:
                subprocess.run(
                    ['git', 'remote', 'add', 'origin', git_url],
                    cwd=module_path,
                    capture_output=True,
                    check=True
                )
                cmd.dbg(f"Added remote 'origin': {git_url}")
                remote_added = True

                # 尝试进行初始提交和推送
                try:
                    # 设置用户信息（如果还未设置）
                    subprocess.run(
                        ['git', 'config', 'user.name', 'OneFramework Setup'],
                        cwd=module_path,
                        capture_output=True,
                    )
                    subprocess.run(
                        ['git', 'config', 'user.email', 'setup@oneframework.local'],
                        cwd=module_path,
                        capture_output=True,
                    )

                    # 添加所有文件
                    subprocess.run(
                        ['git', 'add', '.'],
                        cwd=module_path,
                        capture_output=True,
                        check=True
                    )

                    # 初始提交
                    subprocess.run(
                        ['git', 'commit', '-m', 'init commit'],
                        cwd=module_path,
                        capture_output=True,
                        check=True
                    )
                    cmd.dbg("Initial commit created")

                    # 尝试推送到 origin/main
                    result = subprocess.run(
                        ['git', 'push', '-u', 'origin', 'main'],
                        cwd=module_path,
                        capture_output=True,
                        text=True
                    )

                    if result.returncode == 0:
                        cmd.inf("✓ Successfully pushed to remote")
                    else:
                        # 如果 main 分支不存在，尝试 master
                        result = subprocess.run(
                            ['git', 'push', '-u', 'origin', 'master'],
                            cwd=module_path,
                            capture_output=True,
                            text=True
                        )
                        if result.returncode == 0:
                            cmd.inf("✓ Successfully pushed to remote (master branch)")
                        else:
                            cmd.wrn(f"Could not push to remote: {result.stderr}")

                except subprocess.CalledProcessError as e:
                    cmd.dbg(f"Git operation failed: {e}")

            except subprocess.CalledProcessError as e:
                cmd.wrn(f"Failed to add remote: {e}")

        return remote_added

    except subprocess.CalledProcessError as e:
        cmd.wrn(f"Failed to initialize git repository: {e}")
        return False
    except Exception as e:
        cmd.wrn(f"Error initializing git: {e}")
        return False
