"""
Shared utilities for OneFramework Node commands.
"""

import re
import subprocess
from pathlib import Path
from typing import Optional, Tuple

import yaml


class NameConverter:
    """处理各种命名风格之间的转换"""

    @staticmethod
    def to_kebab_case(name: str) -> str:
        """转换为 kebab-case (my-motor)"""
        # 处理 PascalCase -> kebab-case
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1-\2', name)
        # 处理 snake_case -> kebab-case
        s2 = re.sub('_', '-', s1)
        # 处理多个大写字母
        return re.sub('([a-z0-9])([A-Z])', r'\1-\2', s2).lower()

    @staticmethod
    def to_pascal_case(name: str) -> str:
        """转换为 PascalCase (MyMotor)"""
        # 先转为kebab-case统一格式
        kebab = NameConverter.to_kebab_case(name)
        # 然后转为PascalCase
        return ''.join(word.capitalize() for word in kebab.split('-'))

    @staticmethod
    def to_snake_case(name: str) -> str:
        """转换为 snake_case (my_motor)"""
        kebab = NameConverter.to_kebab_case(name)
        return kebab.replace('-', '_')

    @staticmethod
    def to_upper_snake_case(name: str) -> str:
        """转换为 UPPER_SNAKE_CASE (MY_MOTOR)"""
        return NameConverter.to_snake_case(name).upper()

    @staticmethod
    def validate_name(name: str) -> Tuple[bool, Optional[str]]:
        """验证名称是否合法"""
        if not name:
            return False, "名称不能为空"

        # 检查是否包含非法字符
        if not re.match(r'^[a-zA-Z0-9_-]+$', name):
            return False, "名称只能包含字母、数字、下划线和连字符"

        # 检查是否以数字开头
        if re.match(r'^\d', name):
            return False, "名称不能以数字开头"

        return True, None


class YamlHandler:
    """YAML文件处理工具"""

    @staticmethod
    def load(path: Path) -> dict:
        """加载YAML文件（尽量保留格式）"""
        try:
            from ruamel.yaml import YAML
            yaml_handler = YAML()
            yaml_handler.preserve_quotes = True
            with open(path, 'r') as f:
                return yaml_handler.load(f)
        except ImportError:
            # 降级到标准yaml（会丢失注释）
            with open(path, 'r') as f:
                return yaml.safe_load(f)

    @staticmethod
    def save(path: Path, data: dict):
        """保存YAML文件（尽量保留格式）"""
        try:
            from ruamel.yaml import YAML
            yaml_handler = YAML()
            yaml_handler.preserve_quotes = True
            yaml_handler.default_flow_style = False
            yaml_handler.indent(mapping=2, sequence=2, offset=0)
            with open(path, 'w') as f:
                yaml_handler.dump(data, f)
        except ImportError:
            # 降级到标准yaml
            with open(path, 'w') as f:
                yaml.dump(data, f, default_flow_style=False, sort_keys=False)


class GitHelper:
    """Git操作辅助工具"""

    @staticmethod
    def is_git_url(path: str) -> bool:
        """判断是否是Git URL"""
        return path.startswith(('http://', 'https://', 'git@'))

    @staticmethod
    def parse_github_url(url: str) -> Optional[Tuple[str, str]]:
        """解析GitHub URL，返回(org, repo)"""
        patterns = [
            r'https://github\.com/([^/]+)/(.+?)(?:\.git)?$',
            r'git@github\.com:([^/]+)/(.+?)(?:\.git)?$',
        ]

        for pattern in patterns:
            match = re.match(pattern, url)
            if match:
                return match.groups()

        return None

    @staticmethod
    def get_remote_info(repo_path: Path) -> dict:
        """获取Git仓库的remote信息"""
        try:
            # 获取remote URL
            result = subprocess.run(
                ['git', 'remote', 'get-url', 'origin'],
                cwd=repo_path,
                capture_output=True,
                text=True,
                check=True
            )
            url = result.stdout.strip()

            # 解析URL
            github_info = GitHelper.parse_github_url(url)
            if github_info:
                org, repo = github_info
                return {
                    'remote': 'origin',
                    'repo_path': f'{org}/{repo}',
                    'url': url,
                }

            return {'url': url, 'remote': 'origin'}
        except Exception:
            return {}


class WorkspaceHelper:
    """West workspace辅助工具"""

    @staticmethod
    def find_root(manifest=None) -> Path:
        """查找West workspace根目录"""
        if manifest:
            return Path(manifest.topdir)

        # 向上搜索.west目录
        current = Path.cwd()
        while current != current.parent:
            if (current / '.west').exists():
                return current
            current = current.parent

        # 如果找不到，使用当前目录
        return Path.cwd()

    @staticmethod
    def find_manifest(start_path: Path = None) -> Path:
        """查找west.yml文件"""
        if start_path is None:
            start_path = Path.cwd()

        current = start_path
        while current != current.parent:
            manifest = current / 'west.yml'
            if manifest.exists():
                return manifest
            current = current.parent

        raise FileNotFoundError("Could not find west.yml manifest")


class TemplateEngine:
    """简单的模板替换引擎"""

    @staticmethod
    def render(template: str, context: dict) -> str:
        """使用context中的值替换template中的占位符"""
        result = template
        for key, value in context.items():
            placeholder = f'{{{key}}}'
            result = result.replace(placeholder, str(value))
        return result

    @staticmethod
    def render_file(template_path: Path, output_path: Path, context: dict):
        """从模板文件渲染到输出文件"""
        template_content = template_path.read_text()
        output_content = TemplateEngine.render(template_content, context)
        output_path.write_text(output_content)
