# 一発書き（文章執筆支援アプリ）

ランダムなお題と必須要素を使い、制限時間内に1作品を書き切るためのデスクトップアプリです。

- お題と必須要素を引き、確定したら、書き終わり（または時間切れ・ギブアップ）まで走り切ります。
- 確定後は、お題の再選定・タイマーのリセットはできません。
- 時間切れ・ギブアップ・再起動後の復旧では、タイトルを付けて保存するだけの画面になります。
- 最優先は、堅牢さ（書いた文章を失わない）と軽快さです。

## 動作環境

- Windows 10／11
- 一般的なデスクトップLinux（Qt 6 の実行環境が必要）
- ネットワーク通信・AI機能・GPUは使用しません

## ビルド方法

必要なもの: CMake 3.24以上、C++17コンパイラ、Qt 6（Core / Widgets / Test）

### Linux

```sh
sudo apt install -y cmake ninja-build qt6-base-dev qt6-base-dev-tools
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### Windows（MSVC）

Qt 6（MSVC 64bit）とCMakeをインストールし、QtのbinにPATHを通してから:

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## 実行方法

実行ファイルと同じ場所の `data/prompts.json` をお題データとして読み込みます。配布物では `bin/ippatsugaki`（Windowsは `bin\ippatsugaki.exe`）と `bin/data/prompts.json` の配置を崩さないでください。

```sh
./build/ippatsugaki
```

配布用フォルダの作り方:

```sh
cmake --install build --prefix dist
```

- Windows: `dist/bin/ippatsugaki.exe` に `windeployqt` を実行すると依存DLLが同梱されます。
- Linux: `cpack -G TGZ` で `ippatsugaki-<version>-Linux.tar.gz` ができます。Qt 6 の実行環境が別途必要です。

## お題データ（data/prompts.json）

```json
{
  "version": 1,
  "subjects": ["金魚", "町内会長"],
  "events": ["人類に宣戦布告した", "突然10人に増えた"],
  "elements": ["学校", "赤い傘", "遺言"]
}
```

- `subjects`: 助詞「が」に続けられる名詞句
- `events`: 言い切りの述語
- `elements`: 必須要素（単語または短い名詞句）
- お題は `{主体}が{出来事}。` の形で表示されます。
- UTF-8（BOM許容）。文字列でない要素、空の要素、100文字を超える要素は無視されます。
- ファイルの欠落・形式不正・リストが空の場合は、内蔵データで補い、画面の隅に控えめに通知します。

## 一時保存と復旧

- 一時保存ファイル `draft.txt` はOS標準のアプリ用ローカルデータ領域に作られます。
  - Linux: `~/.local/share/Ippatsugaki/draft.txt`
  - Windows: `%LOCALAPPDATA%\Ippatsugaki\draft.txt`
- 書き込みは本文に変更があった場合に最大30秒ごと、および一時停止時・保存画面に入る直前です。
- 書き込みは `QSaveFile` で行い、書き込み中の異常終了によるファイル破損を防ぎます。
- 起動時に内容のある `draft.txt` があると、復旧するか確認されます。「はい」を選ぶと本文を読み込んで保存画面（ギブアップ扱い）に進みます。続きを書くことはできません。

## テスト

自動テスト（Qt Test / CTest）:

```sh
ctest --test-dir build --output-on-failure
```

- お題生成: 直前と同じ要素が連続しない、データ不足・不正データでの代替動作
- 文字数計算: サロゲートペア、結合文字、各種空白、改行
- 改行コードの正規化
- タイマー: 一時停止・再開、遅延の非累積、スリープ検出（偽の時計を使用）
- 一時保存: 書き込み、復旧、破損・空ファイルの扱い
- 状態遷移: 確定後に引き直せない、リセットが存在しない

手動の受け入れテスト手順は [docs/手動受け入れテスト手順書.md](docs/手動受け入れテスト手順書.md) を参照してください。

## テスト状況（重要）

- 自動テストは、開発環境のLinux上で実施済みです。
- **Windowsでの動作確認は未確認です。**
- **Linux実機での日本語入力（IME）の動作は未確認です。**
- **スリープ復帰時の自動一時停止は、実機では未確認です**（偽の時計による自動テストのみ実施済み）。
- 確認できていないOS・環境を「テスト済み」とは記載しません。

## Linuxでの日本語入力について

- Linuxで日本語を入力するには、Qt用のIME連携（IBus／FcitxのQtプラグインなど）が入っている必要があります。
- 配布物のtar.gzにはQt本体を含みません。Qt 6ランタイムを別途インストールしてください。
- AppImage形式は提供していません（日本語入力との連携が環境によって動かない場合があるため）。

## 対象外

- AIによる文章生成・校正
- インターネット接続、アカウント登録、クラウド同期
- ファイルを開く機能、新規作成機能
- 確定後のお題の再選定、タイマーのリセット
- 本文の自動保存（一時保存ファイル以外）
- リッチテキスト、設定画面、履歴、装飾
