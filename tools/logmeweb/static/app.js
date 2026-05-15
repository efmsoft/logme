let CurrentTarget = null;
let CurrentView = 'overview';
let LastDiscovery = [];
let CachedChannels = null;

function $(id)
{
  return document.getElementById(id);
}

function EscapeHtml(value)
{
  return String(value ?? '')
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#039;');
}

function SetScreen(name)
{
  $('connectScreen').classList.toggle('active', name === 'connect');
  $('mainScreen').classList.toggle('active', name === 'main');
}

function NormalizeTarget(item, password)
{
  const control = item.control || {};

  return {
    pid: item.pid || null,
    process: item.process || 'Manual target',
    instance: item.instance || '',
    version: item.version || '',
    host: control.host || item.host || '127.0.0.1',
    port: Number(control.port || item.port || 0),
    protocol: (control.protocol || item.protocol || 'http').toLowerCase(),
    authRequired: Boolean(item.authRequired),
    password: password || ''
  };
}

function ConnectToTarget(item, password)
{
  CurrentTarget = NormalizeTarget(item, password);
  CurrentView = 'overview';
  CachedChannels = null;
  UpdateTargetSummary();
  SetScreen('main');
  SelectView('overview');
}

function UpdateTargetSummary()
{
  const target = CurrentTarget;

  $('targetSummary').innerHTML = `
    <div class="target-name">${EscapeHtml(target.process)}</div>
    <div class="target-line">${EscapeHtml(target.protocol.toUpperCase())} ${EscapeHtml(target.host)}:${EscapeHtml(target.port)}</div>
    <div class="target-line">PID: ${EscapeHtml(target.pid || 'n/a')}</div>
    <div class="target-line">Control version: ${EscapeHtml(target.version || 'n/a')}</div>
  `;
}

async function LoadDiscovery()
{
  const status = $('discoveryStatus');
  const list = $('discoveryList');

  status.textContent = 'Searching...';
  list.innerHTML = '';

  try
  {
    const response = await fetch('/api/discovery');
    const data = await response.json();
    LastDiscovery = data;

    if (!data.length)
    {
      status.textContent = 'No discoverable logme applications found.';
      return;
    }

    status.textContent = `${data.length} application(s) found.`;

    for (const item of data)
    {
      const control = item.control || {};
      const div = document.createElement('div');
      div.className = 'discovery-item';

      div.innerHTML = `
        <div class="discovery-main">
          <div class="process-name">${EscapeHtml(item.process || 'Unknown process')}</div>
          <div class="process-meta">PID ${EscapeHtml(item.pid)} · ${EscapeHtml((control.protocol || 'http').toUpperCase())} ${EscapeHtml(control.host || '127.0.0.1')}:${EscapeHtml(control.port || '')}</div>
        </div>
        <div class="discovery-auth">
          <input type="password" placeholder="Password${item.authRequired ? '' : ' optional'}">
          <button class="primary">Connect</button>
        </div>
      `;

      div.querySelector('button').addEventListener('click', () =>
      {
        const password = div.querySelector('input').value;
        ConnectToTarget(item, password);
      });

      list.appendChild(div);
    }
  }
  catch (e)
  {
    status.textContent = `Discovery failed: ${e}`;
  }
}

async function SendCommand(command, format = 'text')
{
  if (!CurrentTarget)
    throw new Error('No target selected');

  const response = await fetch('/api/command', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      host: CurrentTarget.host,
      port: CurrentTarget.port,
      protocol: CurrentTarget.protocol,
      password: CurrentTarget.password,
      command: command,
      format: format
    })
  });

  const data = await response.json();

  if (!response.ok)
    throw new Error(data.detail || 'request failed');

  return data;
}

function RenderResponseCard(title, command, result)
{
  const okClass = result.ok ? 'ok' : 'bad';
  const error = result.error ? `<div class="error-text">${EscapeHtml(result.error)}</div>` : '';
  const text = result.text || '';

  return `
    <div class="result-card">
      <div class="result-header">
        <div>
          <div class="result-title">${EscapeHtml(title)}</div>
          <div class="command-line">${EscapeHtml(command)}</div>
        </div>
        <span class="badge ${okClass}">${result.ok ? 'OK' : 'ERROR'}</span>
      </div>
      ${error}
      <pre>${EscapeHtml(text)}</pre>
    </div>
  `;
}

function ChannelNameForDisplay(name)
{
  return name === '' ? '<default>' : name;
}

function QuoteArg(value)
{
  const text = String(value ?? '');
  if (/^[A-Za-z0-9_.:-]+$/.test(text))
    return text;

  return '"' + text.replaceAll('\\', '\\\\').replaceAll('"', '\\"') + '"';
}

function ChannelOptionArg(channel)
{
  if (!channel)
    return '';

  return ` --channel ${QuoteArg(channel)}`;
}

async function LoadChannels()
{
  if (CachedChannels)
    return CachedChannels;

  const result = await SendCommand('list', 'json');
  CachedChannels = result.json?.data?.channels || [];
  return CachedChannels;
}

function RenderTags(items, emptyText)
{
  if (!items.length)
    return `<div class="empty-state small">${EscapeHtml(emptyText)}</div>`;

  return `<div class="tag-list">${items.map(x => `<span class="tag">${EscapeHtml(x)}</span>`).join('')}</div>`;
}

function ParseBackendTypes(channelText)
{
  const result = {};
  const lines = (channelText || '').split(/\r?\n/);

  for (const line of lines)
  {
    const match = line.match(/^\s+([A-Za-z0-9_]+Backend)\b/);
    if (!match)
      continue;

    result[match[1]] = (result[match[1]] || 0) + 1;
  }

  return result;
}

function ParseAccessCount(channelText)
{
  const match = (channelText || '').match(/^Access count:\s*(\d+)/m);
  if (!match)
    return 0;

  return Number(match[1] || 0);
}

function MergeCounters(dst, src)
{
  for (const [key, value] of Object.entries(src))
    dst[key] = (dst[key] || 0) + value;
}

function RenderStatCard(label, value, note)
{
  return `
    <div class="stat-card">
      <div class="stat-label">${EscapeHtml(label)}</div>
      <div class="stat-value">${EscapeHtml(value)}</div>
      ${note ? `<div class="stat-note">${EscapeHtml(note)}</div>` : ''}
    </div>
  `;
}

async function RenderOverview()
{
  $('workArea').innerHTML = '<div class="loading">Loading overview...</div>';

  try
  {
    const channels = await LoadChannels();
    const subsystemResult = await SendCommand('subsystem', 'json');
    const subsystemData = subsystemResult.json?.data || {};
    const blocked = subsystemData.blockedSubsystems || [];
    const allowed = subsystemData.allowedSubsystems || [];

    let accessCount = 0;
    const backendTypes = {};
    const channelCards = [];

    for (const channel of channels)
    {
      const command = channel ? `channel ${QuoteArg(channel)}` : 'channel';
      const detail = await SendCommand(command, 'text');
      accessCount += ParseAccessCount(detail.text || '');
      MergeCounters(backendTypes, ParseBackendTypes(detail.text || ''));
      channelCards.push(RenderResponseCard(ChannelNameForDisplay(channel), command, detail));
    }

    const backendCount = Object.values(backendTypes).reduce((sum, x) => sum + x, 0);
    const backendHtml = Object.keys(backendTypes).length
      ? Object.entries(backendTypes)
        .sort((a, b) => a[0].localeCompare(b[0]))
        .map(([name, count]) => `<div class="metric-line"><span>${EscapeHtml(name)}</span><strong>${EscapeHtml(count)}</strong></div>`)
        .join('')
      : '<div class="empty-state small">No backends found</div>';

    $('workArea').innerHTML = `
      <div class="stats-grid">
        ${RenderStatCard('Channels', channels.length, 'Including the default channel')}
        ${RenderStatCard('Subsystem filter entries', blocked.length + allowed.length, `${blocked.length} blocked, ${allowed.length} allowed`)}
        ${RenderStatCard('Backends', backendCount, 'Grouped by backend type below')}
        ${RenderStatCard('Channel access count', accessCount, 'Sum of current channel access counters')}
        ${RenderStatCard('Logged bytes', 'n/a', 'Current control protocol does not expose total logged bytes yet')}
      </div>

      <div class="cards-grid two-columns">
        <div class="card soft">
          <h2>Backend types</h2>
          <div class="metric-list">${backendHtml}</div>
        </div>
        <div class="card soft">
          <h2>Subsystems</h2>
          <div class="metric-line"><span>Blocked</span><strong>${EscapeHtml(blocked.length)}</strong></div>
          <div class="metric-line"><span>Allowed</span><strong>${EscapeHtml(allowed.length)}</strong></div>
        </div>
      </div>

      <div class="section-title wide">Channel details used for this summary</div>
      <div class="cards-grid">${channelCards.join('')}</div>
    `;
  }
  catch (e)
  {
    $('workArea').innerHTML = `<div class="error-card">${EscapeHtml(e)}</div>`;
  }
}

async function RenderChannels()
{
  $('workArea').innerHTML = '<div class="loading">Loading channels...</div>';

  const channels = await LoadChannels();

  let html = `
    <div class="split-view">
      <div class="list-panel">
        <div class="section-title">Channels</div>
        <div class="hint">Click a channel to request detailed information.</div>
        <div class="item-list" id="channelList">
  `;

  for (const channel of channels)
  {
    html += `
      <button class="list-item" data-channel="${EscapeHtml(channel)}">
        <span>${EscapeHtml(ChannelNameForDisplay(channel))}</span>
      </button>
    `;
  }

  html += `
        </div>
      </div>
      <div class="detail-panel" id="channelDetails">
        <div class="empty-state">Select a channel on the left.</div>
      </div>
    </div>
  `;

  $('workArea').innerHTML = html;

  document.querySelectorAll('#channelList .list-item').forEach(button =>
  {
    button.addEventListener('click', async () =>
    {
      document.querySelectorAll('#channelList .list-item').forEach(x => x.classList.remove('selected'));
      button.classList.add('selected');

      const channel = button.dataset.channel;
      const cmd = channel ? `channel ${QuoteArg(channel)}` : 'channel';
      $('channelDetails').innerHTML = '<div class="loading">Loading channel details...</div>';

      try
      {
        const detail = await SendCommand(cmd, 'text');
        $('channelDetails').innerHTML = RenderResponseCard(ChannelNameForDisplay(channel), cmd, detail);
      }
      catch (e)
      {
        $('channelDetails').innerHTML = `<div class="error-text">${EscapeHtml(e)}</div>`;
      }
    });
  });
}

async function RenderSubsystems()
{
  $('workArea').innerHTML = '<div class="loading">Loading subsystems...</div>';

  const result = await SendCommand('subsystem', 'json');
  const data = result.json?.data || {};
  const blocked = data.blockedSubsystems || [];
  const allowed = data.allowedSubsystems || [];

  $('workArea').innerHTML = `
    <div class="cards-grid">
      <div class="card soft">
        <h2>Blocked subsystems</h2>
        ${RenderTags(blocked, 'No blocked subsystems')}
      </div>
      <div class="card soft">
        <h2>Allowed subsystems</h2>
        ${RenderTags(allowed, 'No allowed subsystems')}
      </div>
    </div>
    ${RenderResponseCard('Raw response', 'subsystem', result)}
  `;
}

async function RenderTracepoints()
{
  $('workArea').innerHTML = '<div class="loading">Loading trace points...</div>';

  const result = await SendCommand('trace', 'text');

  $('workArea').innerHTML = `
    <div class="toolbar">
      <input id="traceFilter" placeholder="Filter trace points...">
    </div>
    <div id="traceOutput" class="result-card">
      <div class="result-header">
        <div>
          <div class="result-title">Trace points</div>
          <div class="command-line">trace</div>
        </div>
        <span class="badge ${result.ok ? 'ok' : 'bad'}">${result.ok ? 'OK' : 'ERROR'}</span>
      </div>
      <pre>${EscapeHtml(result.text || '')}</pre>
    </div>
  `;

  $('traceFilter').addEventListener('input', () =>
  {
    const filter = $('traceFilter').value.toLowerCase();
    const lines = (result.text || '').split(/\r?\n/);
    const filtered = lines.filter(line => line.toLowerCase().includes(filter)).join('\n');
    document.querySelector('#traceOutput pre').textContent = filtered;
  });
}

async function RenderFlags()
{
  $('workArea').innerHTML = '<div class="loading">Loading flags...</div>';

  const channels = await LoadChannels();
  let html = '<div class="cards-grid">';

  for (const channel of channels)
  {
    const channelArg = ChannelOptionArg(channel);
    const flagsCommand = `flags${channelArg}`;
    const levelCommand = `level${channelArg}`;
    const flags = await SendCommand(flagsCommand, 'json');
    const level = await SendCommand(levelCommand, 'json');

    html += `
      <div class="card soft">
        <h2>${EscapeHtml(ChannelNameForDisplay(channel))}</h2>
        <div class="metric-line"><span>Level</span><strong>${EscapeHtml(level.json?.data?.level || 'n/a')}</strong></div>
        <div class="tag-list">${(flags.json?.data?.names || []).map(x => `<span class="tag">${EscapeHtml(x)}</span>`).join('') || '<span class="muted">No named flags</span>'}</div>
        <div class="command-line">${EscapeHtml(flagsCommand)} · ${EscapeHtml(levelCommand)}</div>
      </div>
    `;
  }

  html += '</div>';
  $('workArea').innerHTML = html;
}

function BuildChannelSelect(channels)
{
  return `
    <label>Channel</label>
    <select id="builderChannel">
      ${channels.map(channel => `<option value="${EscapeHtml(channel)}">${EscapeHtml(ChannelNameForDisplay(channel))}</option>`).join('')}
    </select>
    <div class="hint compact">For the default channel the command is sent without --channel.</div>
  `;
}

function SetManualCommand(text)
{
  $('manualCommandText').value = text;
}

function GetBuilderChannelArg()
{
  const channel = $('builderChannel') ? $('builderChannel').value : '';
  return ChannelOptionArg(channel);
}

function UpdateManualBuilder()
{
  const preset = $('commandPreset').value;
  const box = $('commandBuilder');
  const channels = CachedChannels || [''];

  if (preset === 'help')
  {
    box.innerHTML = '<div class="hint">Help is shown here because it is mainly useful while composing manual commands.</div>';
    SetManualCommand('help');
    return;
  }

  if (preset === 'list')
  {
    box.innerHTML = '<div class="hint">Lists all channels.</div>';
    SetManualCommand('list');
    return;
  }

  if (preset === 'channel-info')
  {
    box.innerHTML = BuildChannelSelect(channels);
    const update = () =>
    {
      const channel = $('builderChannel').value;
      SetManualCommand(channel ? `channel ${QuoteArg(channel)}` : 'channel');
    };
    $('builderChannel').addEventListener('change', update);
    update();
    return;
  }

  if (preset === 'channel-op')
  {
    box.innerHTML = `
      <label>Operation</label>
      <select id="builderChannelOp">
        <option value="--create">create</option>
        <option value="--delete">delete</option>
        <option value="--enable">enable</option>
        <option value="--disable">disable</option>
      </select>
      <label>Channel name</label>
      <input id="builderChannelName" placeholder="channel name">
    `;
    const update = () => SetManualCommand(`channel ${$('builderChannelOp').value} ${QuoteArg($('builderChannelName').value)}`);
    $('builderChannelOp').addEventListener('change', update);
    $('builderChannelName').addEventListener('input', update);
    update();
    return;
  }

  if (preset === 'flags')
  {
    box.innerHTML = `
      ${BuildChannelSelect(channels)}
      <label>Flags arguments</label>
      <input id="builderArgs" placeholder="empty = display flags, or flag[=value] ...">
    `;
    const update = () => SetManualCommand(`flags${GetBuilderChannelArg()} ${$('builderArgs').value}`.trim());
    $('builderChannel').addEventListener('change', update);
    $('builderArgs').addEventListener('input', update);
    update();
    return;
  }

  if (preset === 'level')
  {
    box.innerHTML = `
      ${BuildChannelSelect(channels)}
      <label>Level</label>
      <select id="builderLevel">
        <option value="">display current level</option>
        <option value="debug">debug</option>
        <option value="info">info</option>
        <option value="warn">warn</option>
        <option value="error">error</option>
        <option value="critical">critical</option>
      </select>
    `;
    const update = () => SetManualCommand(`level${GetBuilderChannelArg()} ${$('builderLevel').value}`.trim());
    $('builderChannel').addEventListener('change', update);
    $('builderLevel').addEventListener('change', update);
    update();
    return;
  }

  if (preset === 'backend')
  {
    box.innerHTML = `
      ${BuildChannelSelect(channels)}
      <label>Operation</label>
      <select id="builderBackendOp">
        <option value="--add">add</option>
        <option value="--delete">delete</option>
      </select>
      <label>Backend type</label>
      <select id="builderBackendType">
        <option value="ConsoleBackend">ConsoleBackend</option>
        <option value="DebugBackend">DebugBackend</option>
        <option value="FileBackend">FileBackend</option>
        <option value="SharedFileBackend">SharedFileBackend</option>
        <option value="BufferBackend">BufferBackend</option>
      </select>
    `;
    const update = () => SetManualCommand(`backend${GetBuilderChannelArg()} ${$('builderBackendOp').value} ${$('builderBackendType').value}`);
    $('builderChannel').addEventListener('change', update);
    $('builderBackendOp').addEventListener('change', update);
    $('builderBackendType').addEventListener('change', update);
    update();
    return;
  }

  if (preset === 'subsystem')
  {
    box.innerHTML = `
      <label>Operation</label>
      <select id="builderSubsystemOp">
        <option value="">display current filters</option>
        <option value="--block">block</option>
        <option value="--unblock">unblock</option>
        <option value="--allow">allow</option>
        <option value="--disallow">disallow</option>
        <option value="--check">check</option>
        <option value="--clear">clear all</option>
        <option value="--clear-blocked">clear blocked</option>
        <option value="--clear-allowed">clear allowed</option>
      </select>
      <label>Subsystem name</label>
      <input id="builderSubsystemName" placeholder="used by block/unblock/allow/disallow/check">
    `;
    const update = () =>
    {
      const op = $('builderSubsystemOp').value;
      const name = $('builderSubsystemName').value;
      const needsName = ['--block', '--unblock', '--allow', '--disallow', '--check'].includes(op);
      SetManualCommand(`subsystem ${op}${needsName ? ' ' + QuoteArg(name) : ''}`.trim());
    };
    $('builderSubsystemOp').addEventListener('change', update);
    $('builderSubsystemName').addEventListener('input', update);
    update();
    return;
  }

  if (preset === 'trace')
  {
    box.innerHTML = `
      <label>Operation</label>
      <select id="builderTraceOp">
        <option value="">list</option>
        <option value="list">list</option>
        <option value="stat">stat</option>
        <option value="enable">enable</option>
        <option value="disable">disable</option>
        <option value="reset">reset</option>
      </select>
      <label>Pattern</label>
      <input id="builderTracePattern" placeholder="optional wildcard pattern">
    `;
    const update = () => SetManualCommand(`trace ${$('builderTraceOp').value} ${$('builderTracePattern').value}`.trim());
    $('builderTraceOp').addEventListener('change', update);
    $('builderTracePattern').addEventListener('input', update);
    update();
    return;
  }

  box.innerHTML = '<div class="hint">Custom mode leaves the command text fully editable.</div>';
  SetManualCommand('');
}

async function RenderManual()
{
  $('workArea').innerHTML = '<div class="loading">Loading command builder...</div>';

  try
  {
    await LoadChannels();
  }
  catch (e)
  {
    CachedChannels = [''];
  }

  $('workArea').innerHTML = `
    <div class="manual-layout">
      <div class="manual-command card soft">
        <h2>Manual command</h2>
        <p>Select a command and fill the fields. The final command text remains editable before sending.</p>

        <label>Command preset</label>
        <select id="commandPreset">
          <option value="help">help</option>
          <option value="list">list</option>
          <option value="channel-info">channel information</option>
          <option value="channel-op">channel operation</option>
          <option value="flags">flags</option>
          <option value="level">level</option>
          <option value="backend">backend</option>
          <option value="subsystem">subsystem</option>
          <option value="trace">trace</option>
          <option value="custom">custom</option>
        </select>

        <div id="commandBuilder" class="command-builder"></div>

        <label>Command text</label>
        <input id="manualCommandText" value="help">

        <label>Format</label>
        <select id="manualFormat">
          <option value="text">Text</option>
          <option value="json">JSON</option>
        </select>

        <button class="primary" id="sendManualButton">Send command</button>
      </div>

      <div id="manualResult" class="manual-result"></div>
    </div>
  `;

  $('commandPreset').addEventListener('change', UpdateManualBuilder);
  UpdateManualBuilder();

  $('sendManualButton').addEventListener('click', async () =>
  {
    const command = $('manualCommandText').value;
    const format = $('manualFormat').value;
    $('manualResult').innerHTML = '<div class="loading">Sending command...</div>';

    try
    {
      const result = await SendCommand(command, format);
      $('manualResult').innerHTML = RenderResponseCard('Manual command result', command, result);
    }
    catch (e)
    {
      $('manualResult').innerHTML = `<div class="error-text">${EscapeHtml(e)}</div>`;
    }
  });
}

async function SelectView(view)
{
  CurrentView = view;

  document.querySelectorAll('.nav-item').forEach(item =>
  {
    item.classList.toggle('active', item.dataset.view === view);
  });

  const titleMap = {
    overview: 'Overview',
    channels: 'Channels',
    subsystems: 'Subsystems',
    tracepoints: 'Trace points',
    flags: 'Flags and level',
    manual: 'Manual command'
  };

  $('viewTitle').textContent = titleMap[view] || view;
  $('viewEyebrow').textContent = `${CurrentTarget.process} · ${CurrentTarget.host}:${CurrentTarget.port}`;

  try
  {
    if (view === 'overview')
      await RenderOverview();
    else if (view === 'channels')
      await RenderChannels();
    else if (view === 'subsystems')
      await RenderSubsystems();
    else if (view === 'tracepoints')
      await RenderTracepoints();
    else if (view === 'flags')
      await RenderFlags();
    else if (view === 'manual')
      await RenderManual();
  }
  catch (e)
  {
    $('workArea').innerHTML = `<div class="error-card">${EscapeHtml(e)}</div>`;
  }
}

function SetupEvents()
{
  $('refreshDiscoveryButton').addEventListener('click', LoadDiscovery);

  $('manualConnectButton').addEventListener('click', () =>
  {
    ConnectToTarget(
      {
        process: 'Manual target',
        control: {
          host: $('manualHost').value,
          port: Number($('manualPort').value),
          protocol: $('manualProtocol').value
        }
      },
      $('manualPassword').value
    );
  });

  $('disconnectButton').addEventListener('click', () =>
  {
    CurrentTarget = null;
    CachedChannels = null;
    SetScreen('connect');
    LoadDiscovery();
  });

  $('reloadViewButton').addEventListener('click', () =>
  {
    CachedChannels = null;
    SelectView(CurrentView);
  });

  document.querySelectorAll('.nav-item').forEach(item =>
  {
    item.addEventListener('click', () => SelectView(item.dataset.view));
  });
}

window.addEventListener('load', () =>
{
  SetupEvents();
  LoadDiscovery();
});
