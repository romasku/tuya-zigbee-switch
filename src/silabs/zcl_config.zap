{
  "fileFormat": 2,
  "featureLevel": 106,
  "creator": "zap",
  "keyValuePairs": [
    {
      "key": "commandDiscovery",
      "value": "1"
    },
    {
      "key": "defaultResponsePolicy",
      "value": "always"
    },
    {
      "key": "manufacturerCodes",
      "value": "0x1049"
    }
  ],
  "package": [
    {
      "pathRelativity": "relativeToZap",
      "path": "../../silabs_tools/gecko_sdk/app/zcl/zcl-zap.json",
      "type": "zcl-properties",
      "category": "zigbee",
      "version": 1,
      "description": "Zigbee Silabs ZCL data"
    },
    {
      "pathRelativity": "relativeToZap",
      "path": "../../silabs_tools/gecko_sdk/protocol/zigbee/app/framework/gen-template/gen-templates.json",
      "type": "gen-templates-json",
      "category": "zigbee",
      "version": "zigbee-v0"
    },
    {
      "pathRelativity": "resolveEnvVars",
      "path": "$CUSTOM_ZCL_XML",
      "type": "zcl-xml-standalone"
    }
  ],
    "endpointTypes": [
    {
      "id": 1,
      "name": "Anonymous Endpoint Type",
      "deviceTypeRef": {
        "code": 65535,
        "profileId": 65535,
        "label": "Custom ZCL Device Type",
        "name": "Custom ZCL Device Type",
        "deviceTypeOrder": 0
      },
      "deviceTypes": [
        {
          "code": 65535,
          "profileId": 65535,
          "label": "Custom ZCL Device Type",
          "name": "Custom ZCL Device Type",
          "deviceTypeOrder": 0
        }
      ],
      "deviceVersions": [
        1
      ],
      "deviceIdentifiers": [
        65535
      ],
      "deviceTypeName": "Custom ZCL Device Type",
      "deviceTypeCode": 65535,
      "deviceTypeProfileId": 65535,
      "clusters": [
        {
          "name": "Basic",
          "code": 0,
          "mfgCode": null,
          "define": "BASIC_CLUSTER",
          "side": "server",
          "enabled": 1,
          "attributes": [
            {
              "name": "ZCL version",
              "code": 0,
              "mfgCode": null,
              "side": "server",
              "type": "int8u",
              "included": 1,
              "storageOption": "RAM",
              "singleton": 1,
              "bounded": null,
              "defaultValue": "8",
              "reportable": 0,
              "minInterval": 1,
              "maxInterval": 65534,
              "reportableChange": 0
            },
            {
              "name": "manufacturer name",
              "code": 4,
              "mfgCode": null,
              "side": "server",
              "type": "char_string",
              "included": 1,
              "storageOption": "External",
              "singleton": 1,
              "bounded": 0,
              "defaultValue": "CUSTOM",
              "reportable": 0,
              "minInterval": 1,
              "maxInterval": 65534,
              "reportableChange": 0
            },
            {
              "name": "device config",
              "code": 65280,
              "mfgCode": null,
              "side": "server",
              "type": "char_string",
              "included": 1,
              "storageOption": "External",
              "singleton": 1,
              "bounded": 0,
              "defaultValue": "",
              "reportable": 0,
              "minInterval": 1,
              "maxInterval": 65534,
              "reportableChange": 0
            }
          ]
        }
      ]
    }
  ],
  "endpoints": [
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 1,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 2,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex":0,
      "profileId": 260,
      "endpointId": 3,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 4,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 5,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 6,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 7,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 8,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 9,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 10,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 11,
      "networkId": 0,
      "parentEndpointIdentifier": null
    },
    {
      "endpointTypeName": "Anonymous Endpoint Type",
      "endpointTypeIndex": 0,
      "profileId": 260,
      "endpointId": 12,
      "networkId": 0,
      "parentEndpointIdentifier": null
    }
  ]
}